/**
 * Lock-free single-producer/single-consumer ring buffer over POSIX shared memory.
 *
 * Allows two separate processes to communicate without locks by using a
 * circular buffer with atomic head/tail indices in a mmap'd shared memory
 * region (shm_open + mmap MAP_SHARED).
 *
 * Lock-free guarantee: exactly ONE writer (pushes at head) and ONE reader
 * (pops at tail). Since they operate on different indices, no mutex is needed.
 * The ring is full when tail == (head + 1) % capacity, empty when head == tail.
 *
 * Capacity rounds up to a power of two internally. One slot is always reserved
 * so usable capacity = allocated slots - 1.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

/**
 * Shared-memory single-producer / single-consumer byte ring.
 *
 * The ring stores variable-size frames in fixed-size slots. It is intended for
 * cross-process IPC where exactly one writer and one reader use a given ring.
 */
class ShmSpscRing {
public:
    ShmSpscRing() = default;
    ~ShmSpscRing();

    ShmSpscRing(const ShmSpscRing&) = delete;
    ShmSpscRing& operator=(const ShmSpscRing&) = delete;
    ShmSpscRing(ShmSpscRing&& other) noexcept;
    ShmSpscRing& operator=(ShmSpscRing&& other) noexcept;

    /**
     * @brief Creates a new SHM ring (shm_open O_CREAT). 
     * 
     * Caller owns the SHM region and is responsible for unlinking it.
     * 
     * @param name Name of the shared memory object (e.g. "/my_ring").
     * @param requestedCapacity Number of slots requested (will be rounded up to power of 2).
     * @param maxPayloadBytes Max bytes per slot for payload.
     * @param unlinkOnDestroy If true, shm_unlink is called on destruction.
     * @return Expected containing the initialized ring, or an error string.
     */
    static std::expected<ShmSpscRing, std::string> create(std::string name,
                                                           std::size_t requestedCapacity,
                                                           std::size_t maxPayloadBytes,
                                                           bool unlinkOnDestroy = false);

    /**
     * @brief Opens an existing SHM ring created by another process (shm_open O_RDWR).
     * 
     * @param name Name of the shared memory object to open.
     * @return Expected containing the initialized ring, or an error string.
     */
    static std::expected<ShmSpscRing, std::string> open(std::string name);

    /**
     * @brief Pushes a payload into the ring (writer side).
     * 
     * Uses memory_order_acquire on tail and memory_order_release on head
     * to safely publish new data without locks.
     * 
     * @param payload The data byte span to push.
     * @param overwriteOldest If true, forcefully overwrites easiest slot if full.
     * @return true on success, false if the ring is full.
     */
    bool tryPush(std::span<const std::byte> payload, bool overwriteOldest = false) noexcept;
    bool tryPush(std::string_view payload, bool overwriteOldest = false) noexcept;

    /**
     * @brief Pops the oldest payload from the ring (reader side).
     * 
     * Uses memory_order_acquire on head and memory_order_release on tail
     * to safely consume published data without locks.
     * 
     * @param out Vector or string populated with the popped data.
     * @return true on success, false if the ring is empty.
     */
    bool tryPop(std::vector<std::byte>& out) noexcept;
    bool tryPop(std::string& out) noexcept;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t maxPayloadBytes() const noexcept;

    bool valid() const noexcept;
    const std::string& name() const noexcept;
    void unlink() noexcept;

private:
    struct Header;
    struct SlotHeader;

    ShmSpscRing(std::string name,
                int fd,
                void* mapping,
                std::size_t mappingBytes,
                Header* header,
                bool unlinkOnDestroy);

    void reset() noexcept;
    std::size_t toIndex(std::uint64_t idx) const noexcept;
    SlotHeader* slotHeader(std::size_t index) const noexcept;
    std::byte* slotPayload(SlotHeader* slot) const noexcept;
    const std::byte* slotPayload(const SlotHeader* slot) const noexcept;
    bool popDiscardOldest() noexcept;

    std::string name_{};
    int fd_{-1};
    void* mapping_{nullptr};
    std::size_t mappingBytes_{0};
    Header* header_{nullptr};
    bool unlinkOnDestroy_{false};
};
