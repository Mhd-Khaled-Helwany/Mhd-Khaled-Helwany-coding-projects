/**
 * Implementation of the POSIX shared memory SPSC ring buffer.
 *
 * Memory layout in the SHM region:
 *   [Header: head(atomic), tail(atomic), slotCount, slotSize]
 *   [Slot 0: 4-byte length prefix + payload bytes]
 *   [Slot 1: ...]
 *   ...
 *
 * create() allocates a new SHM region via shm_open(O_CREAT|O_EXCL) + mmap.
 * open() attaches to an existing region via shm_open(O_RDWR) + mmap.
 *
 * tryPush: writes payload into slot at head, bumps head atomically (release).
 * tryPop:  reads payload from slot at tail, bumps tail atomically (release).
 * Both use acquire/release ordering to ensure the payload is visible before
 * the index update is observed by the other side.
 */
#include "engine/ShmSpscRing.hpp"

#include <atomic>
#include <bit>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <new>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace {

constexpr std::uint32_t kRingMagic = 0x53505343; // "SPSC"
constexpr std::uint32_t kRingVersion = 1;
constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;

std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

std::string normalizeShmName(std::string_view name) {
    if (name.empty()) {
        return {};
    }
    if (name.front() == '/') {
        return std::string(name);
    }
    std::string normalized;
    normalized.reserve(name.size() + 1);
    normalized.push_back('/');
    normalized.append(name);
    return normalized;
}

std::string makeErrorMessage(std::string_view prefix, int err) {
    std::error_code ec(err, std::system_category());
    return std::string(prefix) + ": " + ec.message() + " (" + std::to_string(err) + ")";
}

} // namespace

struct alignas(kCacheLineSize) ShmSpscRing::Header {
    std::uint32_t magic{0};
    std::uint32_t version{0};
    std::uint32_t slotPayloadBytes{0};
    std::uint32_t slotCount{0};
    std::uint32_t slotStride{0};
    std::uint32_t reserved{0};
    alignas(kCacheLineSize) std::atomic<std::uint64_t> head;
    alignas(kCacheLineSize) std::atomic<std::uint64_t> tail;
};

struct ShmSpscRing::SlotHeader {
    std::uint32_t size{0};
    std::uint32_t reserved{0};
};

ShmSpscRing::ShmSpscRing(std::string name,
                         int fd,
                         void* mapping,
                         std::size_t mappingBytes,
                         Header* header,
                         bool unlinkOnDestroy)
    : name_(std::move(name))
    , fd_(fd)
    , mapping_(mapping)
    , mappingBytes_(mappingBytes)
    , header_(header)
    , unlinkOnDestroy_(unlinkOnDestroy) {}

ShmSpscRing::~ShmSpscRing() {
    reset();
}

ShmSpscRing::ShmSpscRing(ShmSpscRing&& other) noexcept {
    *this = std::move(other);
}

ShmSpscRing& ShmSpscRing::operator=(ShmSpscRing&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    reset();
    name_ = std::move(other.name_);
    fd_ = other.fd_;
    mapping_ = other.mapping_;
    mappingBytes_ = other.mappingBytes_;
    header_ = other.header_;
    unlinkOnDestroy_ = other.unlinkOnDestroy_;
    other.fd_ = -1;
    other.mapping_ = nullptr;
    other.mappingBytes_ = 0;
    other.header_ = nullptr;
    other.unlinkOnDestroy_ = false;
    return *this;
}

std::expected<ShmSpscRing, std::string> ShmSpscRing::create(std::string name,
                                                            std::size_t requestedCapacity,
                                                            std::size_t maxPayloadBytes,
                                                            bool unlinkOnDestroy) {
    name = normalizeShmName(name);
    if (name.empty()) {
        return std::unexpected("shm ring create: empty name");
    }
    if (requestedCapacity < 2) {
        return std::unexpected("shm ring create: requestedCapacity must be >= 2");
    }
    if (maxPayloadBytes == 0) {
        return std::unexpected("shm ring create: maxPayloadBytes must be > 0");
    }

    const auto slotCount = static_cast<std::uint32_t>(std::bit_ceil(requestedCapacity));
    const auto slotStride = static_cast<std::uint32_t>(
        alignUp(sizeof(SlotHeader) + maxPayloadBytes, kCacheLineSize));
    const auto mappingBytes = alignUp(sizeof(Header), kCacheLineSize) + std::size_t(slotCount) * slotStride;

    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        return std::unexpected(makeErrorMessage("shm_open create failed", errno));
    }

    if (ftruncate(fd, static_cast<off_t>(mappingBytes)) == -1) {
        const std::string error = makeErrorMessage("ftruncate failed", errno);
        close(fd);
        shm_unlink(name.c_str());
        return std::unexpected(error);
    }

    void* mapping = mmap(nullptr, mappingBytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        const std::string error = makeErrorMessage("mmap create failed", errno);
        close(fd);
        shm_unlink(name.c_str());
        return std::unexpected(error);
    }

    std::memset(mapping, 0, mappingBytes);
    auto* header = static_cast<Header*>(mapping);
    header->magic = kRingMagic;
    header->version = kRingVersion;
    header->slotPayloadBytes = static_cast<std::uint32_t>(maxPayloadBytes);
    header->slotCount = slotCount;
    header->slotStride = slotStride;
    std::construct_at(&header->head, 0);
    std::construct_at(&header->tail, 0);

    return ShmSpscRing(std::move(name), fd, mapping, mappingBytes, header, unlinkOnDestroy);
}

std::expected<ShmSpscRing, std::string> ShmSpscRing::open(std::string name) {
    name = normalizeShmName(name);
    if (name.empty()) {
        return std::unexpected("shm ring open: empty name");
    }

    const int fd = shm_open(name.c_str(), O_RDWR, 0600);
    if (fd == -1) {
        return std::unexpected(makeErrorMessage("shm_open open failed", errno));
    }

    struct stat st {};
    if (fstat(fd, &st) == -1) {
        const std::string error = makeErrorMessage("fstat failed", errno);
        close(fd);
        return std::unexpected(error);
    }
    if (st.st_size < static_cast<off_t>(sizeof(Header))) {
        close(fd);
        return std::unexpected("shm ring open: mapping too small");
    }

    void* mapping = mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        const std::string error = makeErrorMessage("mmap open failed", errno);
        close(fd);
        return std::unexpected(error);
    }

    auto* header = static_cast<Header*>(mapping);
    if (header->magic != kRingMagic || header->version != kRingVersion) {
        munmap(mapping, static_cast<std::size_t>(st.st_size));
        close(fd);
        return std::unexpected("shm ring open: invalid header");
    }

    return ShmSpscRing(std::move(name), fd, mapping, static_cast<std::size_t>(st.st_size), header, false);
}

bool ShmSpscRing::tryPush(std::span<const std::byte> payload, bool overwriteOldest) noexcept {
    if (!header_ || payload.size() > header_->slotPayloadBytes) {
        return false;
    }

    auto tail = header_->tail.load(std::memory_order_acquire);
    auto head = header_->head.load(std::memory_order_relaxed);
    const auto usableCapacity = std::uint64_t(header_->slotCount - 1);
    if (head - tail == usableCapacity) {
        if (!overwriteOldest) {
            return false;
        }
        if (!popDiscardOldest()) {
            return false;
        }
        head = header_->head.load(std::memory_order_relaxed);
    }

    auto* slot = slotHeader(toIndex(head));
    slot->size = static_cast<std::uint32_t>(payload.size());
    std::memcpy(slotPayload(slot), payload.data(), payload.size());
    header_->head.store(head + 1, std::memory_order_release);
    return true;
}

bool ShmSpscRing::tryPush(std::string_view payload, bool overwriteOldest) noexcept {
    const auto* ptr = reinterpret_cast<const std::byte*>(payload.data());
    return tryPush(std::span<const std::byte>(ptr, payload.size()), overwriteOldest);
}

bool ShmSpscRing::tryPop(std::vector<std::byte>& out) noexcept {
    if (!header_) {
        return false;
    }

    auto tail = header_->tail.load(std::memory_order_relaxed);
    const auto head = header_->head.load(std::memory_order_acquire);
    if (head == tail) {
        return false;
    }

    auto* slot = slotHeader(toIndex(tail));
    out.resize(slot->size);
    std::memcpy(out.data(), slotPayload(slot), slot->size);
    header_->tail.store(tail + 1, std::memory_order_release);
    return true;
}

bool ShmSpscRing::tryPop(std::string& out) noexcept {
    if (!header_) {
        return false;
    }

    auto tail = header_->tail.load(std::memory_order_relaxed);
    const auto head = header_->head.load(std::memory_order_acquire);
    if (head == tail) {
        return false;
    }

    auto* slot = slotHeader(toIndex(tail));
    out.resize(slot->size);
    std::memcpy(out.data(), slotPayload(slot), slot->size);
    header_->tail.store(tail + 1, std::memory_order_release);
    return true;
}

std::size_t ShmSpscRing::size() const noexcept {
    if (!header_) {
        return 0;
    }
    return static_cast<std::size_t>(
        header_->head.load(std::memory_order_acquire) - header_->tail.load(std::memory_order_acquire));
}

std::size_t ShmSpscRing::capacity() const noexcept {
    return header_ ? static_cast<std::size_t>(header_->slotCount - 1) : 0;
}

std::size_t ShmSpscRing::maxPayloadBytes() const noexcept {
    return header_ ? static_cast<std::size_t>(header_->slotPayloadBytes) : 0;
}

bool ShmSpscRing::valid() const noexcept {
    return header_ != nullptr;
}

const std::string& ShmSpscRing::name() const noexcept {
    return name_;
}

void ShmSpscRing::unlink() noexcept {
    if (!name_.empty()) {
        shm_unlink(name_.c_str());
    }
}

void ShmSpscRing::reset() noexcept {
    if (mapping_) {
        munmap(mapping_, mappingBytes_);
        mapping_ = nullptr;
    }
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
    if (unlinkOnDestroy_ && !name_.empty()) {
        shm_unlink(name_.c_str());
    }
    mappingBytes_ = 0;
    header_ = nullptr;
    unlinkOnDestroy_ = false;
    name_.clear();
}

std::size_t ShmSpscRing::toIndex(std::uint64_t idx) const noexcept {
    return static_cast<std::size_t>(idx & std::uint64_t(header_->slotCount - 1));
}

ShmSpscRing::SlotHeader* ShmSpscRing::slotHeader(std::size_t index) const noexcept {
    auto* base = static_cast<std::byte*>(mapping_) + alignUp(sizeof(Header), kCacheLineSize);
    return reinterpret_cast<SlotHeader*>(base + index * header_->slotStride);
}

std::byte* ShmSpscRing::slotPayload(SlotHeader* slot) const noexcept {
    return reinterpret_cast<std::byte*>(slot) + sizeof(SlotHeader);
}

const std::byte* ShmSpscRing::slotPayload(const SlotHeader* slot) const noexcept {
    return reinterpret_cast<const std::byte*>(slot) + sizeof(SlotHeader);
}

bool ShmSpscRing::popDiscardOldest() noexcept {
    if (!header_) {
        return false;
    }

    auto tail = header_->tail.load(std::memory_order_relaxed);
    const auto head = header_->head.load(std::memory_order_acquire);
    if (head == tail) {
        return false;
    }
    header_->tail.store(tail + 1, std::memory_order_release);
    return true;
}
