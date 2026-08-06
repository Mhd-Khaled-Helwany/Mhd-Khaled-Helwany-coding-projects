/*
Build & run
-----------

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target ShmSpscRingTests
ctest --test-dir build -R ShmSpscRingTests -V
*/

#include "engine/ShmSpscRing.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

struct TestContext {
    int failed = 0;
    int passed = 0;
    std::ostringstream os;
} ctx;

#define TEST_CASE(name) void name()

#define REQUIRE_TRUE(cond)                                                                              \
    do {                                                                                                \
        if (!(cond)) {                                                                                  \
            ++ctx.failed;                                                                               \
            ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": REQUIRE_TRUE(" #cond ") failed\n";   \
            return;                                                                                     \
        }                                                                                               \
    } while (0)

#define EXPECT_TRUE(cond)                                                                               \
    do {                                                                                                \
        if (cond) {                                                                                     \
            ++ctx.passed;                                                                               \
        } else {                                                                                        \
            ++ctx.failed;                                                                               \
            ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": EXPECT_TRUE(" #cond ") failed\n";    \
        }                                                                                               \
    } while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

template <class A, class B>
static void expect_eq_impl(const A& a, const B& b, const char* aExpr, const char* bExpr, const char* func, int line) {
    if (a == b) {
        ++ctx.passed;
    } else {
        ++ctx.failed;
        ctx.os << func << ": line " << line << ": EXPECT_EQ(" << aExpr << ", " << bExpr << ") failed: "
               << a << " != " << b << "\n";
    }
}

#define EXPECT_EQ(a, b) expect_eq_impl((a), (b), #a, #b, __FUNCTION__, __LINE__)

static std::string unique_name(const char* suffix) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string("/shm_spsc_test_") + suffix + "_" + std::to_string(getpid()) + "_" + std::to_string(now);
}

static std::vector<std::byte> bytes_from_string(std::string_view text) {
    const auto* ptr = reinterpret_cast<const std::byte*>(text.data());
    return std::vector<std::byte>(ptr, ptr + text.size());
}

static std::string string_from_bytes(const std::vector<std::byte>& bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

static void run_test_case(const char* name, void (*testFn)()) {
    const int failedBefore = ctx.failed;
    const auto start = std::chrono::steady_clock::now();
    testFn();
    const auto end = std::chrono::steady_clock::now();
    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "[ShmSpscRingTests] " << name << " "
              << ((ctx.failed == failedBefore) ? "completed" : "failed")
              << " in " << elapsedUs << " us\n";
}

/**
 * Test: create_open_roundtrip_and_capacity
 * - Arrange: Create a shared-memory ring and open it from a second handle.
 * - Act: Push two strings from the owner and pop them from the peer.
 * - Assert: Both handles observe the same ring state, FIFO ordering is preserved,
 *           and size/capacity bookkeeping matches the in-process SPSC tests.
 */
TEST_CASE(test_create_open_roundtrip_and_capacity) {
    const std::string name = unique_name("roundtrip");
    auto owner = ShmSpscRing::create(name, 8, 256, true);
    REQUIRE_TRUE(owner.has_value());
    auto peer = ShmSpscRing::open(name);
    REQUIRE_TRUE(peer.has_value());

    EXPECT_TRUE(owner->valid());
    EXPECT_TRUE(peer->valid());
    EXPECT_EQ(owner->name(), name);
    EXPECT_EQ(peer->name(), name);
    EXPECT_EQ(owner->capacity(), static_cast<std::size_t>(7));
    EXPECT_EQ(peer->capacity(), static_cast<std::size_t>(7));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(0));
    EXPECT_EQ(peer->size(), static_cast<std::size_t>(0));

    EXPECT_TRUE(owner->tryPush("alpha"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(1));
    EXPECT_EQ(peer->size(), static_cast<std::size_t>(1));

    EXPECT_TRUE(owner->tryPush("bravo"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(2));
    EXPECT_EQ(peer->size(), static_cast<std::size_t>(2));

    std::string value;
    REQUIRE_TRUE(peer->tryPop(value));
    EXPECT_EQ(value, std::string("alpha"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(1));

    REQUIRE_TRUE(peer->tryPop(value));
    EXPECT_EQ(value, std::string("bravo"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(0));
    EXPECT_FALSE(peer->tryPop(value));
}

/**
 * Test: push_until_full_then_reject
 * - Arrange: Fill the ring up to its usable capacity.
 * - Act: Try pushing one more message without overwrite enabled.
 * - Assert: The push is rejected and the queued messages remain untouched.
 */
TEST_CASE(test_push_until_full_then_reject) {
    const std::string name = unique_name("full");
    auto ring = ShmSpscRing::create(name, 4, 64, true);
    REQUIRE_TRUE(ring.has_value());

    EXPECT_EQ(ring->capacity(), static_cast<std::size_t>(3));
    EXPECT_TRUE(ring->tryPush("one"));
    EXPECT_TRUE(ring->tryPush("two"));
    EXPECT_TRUE(ring->tryPush("three"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(3));

    EXPECT_FALSE(ring->tryPush("four"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(3));
}

/**
 * Test: repeated_pop_preserves_fifo_and_size_updates
 * - Arrange: Push three messages into the ring.
 * - Act: Pop them one-by-one.
 * - Assert: Although ShmSpscRing has no bulk-pop API, repeated pops preserve
 *           the same FIFO ordering and size transitions that SpscRingTest checks.
 */
TEST_CASE(test_repeated_pop_preserves_fifo_and_size_updates) {
    const std::string name = unique_name("fifo");
    auto ring = ShmSpscRing::create(name, 8, 64, true);
    REQUIRE_TRUE(ring.has_value());

    REQUIRE_TRUE(ring->tryPush("0"));
    REQUIRE_TRUE(ring->tryPush("1"));
    REQUIRE_TRUE(ring->tryPush("2"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(3));

    std::string value;
    REQUIRE_TRUE(ring->tryPop(value));
    EXPECT_EQ(value, std::string("0"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(2));

    REQUIRE_TRUE(ring->tryPop(value));
    EXPECT_EQ(value, std::string("1"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(1));

    REQUIRE_TRUE(ring->tryPop(value));
    EXPECT_EQ(value, std::string("2"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(0));

    EXPECT_FALSE(ring->tryPop(value));
}

/**
 * Test: wraparound_preserves_order_across_handles
 * - Arrange: Push, partially drain from a second handle, then push again so the
 *           producer wraps around to reused slots.
 * - Act: Drain the remaining messages from the peer.
 * - Assert: FIFO ordering is preserved across the wrap boundary for both handles.
 */
TEST_CASE(test_wraparound_preserves_order_across_handles) {
    const std::string name = unique_name("wrap");
    auto owner = ShmSpscRing::create(name, 8, 128, true);
    REQUIRE_TRUE(owner.has_value());
    auto peer = ShmSpscRing::open(name);
    REQUIRE_TRUE(peer.has_value());

    for (const std::string& msg : {"0", "1", "2", "3", "4"}) {
        REQUIRE_TRUE(owner->tryPush(msg));
    }
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(5));

    std::string value;
    REQUIRE_TRUE(peer->tryPop(value));
    EXPECT_EQ(value, std::string("0"));
    REQUIRE_TRUE(peer->tryPop(value));
    EXPECT_EQ(value, std::string("1"));
    REQUIRE_TRUE(peer->tryPop(value));
    EXPECT_EQ(value, std::string("2"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(2));

    REQUIRE_TRUE(owner->tryPush("5"));
    REQUIRE_TRUE(owner->tryPush("6"));
    REQUIRE_TRUE(owner->tryPush("7"));
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(5));

    const std::vector<std::string> expected{"3", "4", "5", "6", "7"};
    for (const auto& exp : expected) {
        REQUIRE_TRUE(peer->tryPop(value));
        EXPECT_EQ(value, exp);
    }
    EXPECT_EQ(owner->size(), static_cast<std::size_t>(0));
}

/**
 * Test: overwrite_drops_oldest
 * - Arrange: Fill the ring completely.
 * - Act: Push one more message with overwriteOldest=true.
 * - Assert: The oldest queued message is discarded, the new message is retained,
 *           and the ring stays at full usable capacity. This also mirrors the
 *           string-overwrite behavior covered in SpscRingTest.
 */
TEST_CASE(test_overwrite_drops_oldest) {
    const std::string name = unique_name("overwrite");
    auto ring = ShmSpscRing::create(name, 4, 64, true);
    REQUIRE_TRUE(ring.has_value());

    REQUIRE_TRUE(ring->tryPush("one"));
    REQUIRE_TRUE(ring->tryPush("two"));
    REQUIRE_TRUE(ring->tryPush("three"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(3));

    EXPECT_FALSE(ring->tryPush("ignored"));
    EXPECT_TRUE(ring->tryPush("latest", true));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(3));

    std::string value;
    const std::vector<std::string> expected{"two", "three", "latest"};
    for (const auto& exp : expected) {
        REQUIRE_TRUE(ring->tryPop(value));
        EXPECT_EQ(value, exp);
    }
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(0));
}

/**
 * Test: string_lvalue_and_temporary_pushes
 * - Arrange: Push strings through the same API shapes that SpscRingTest covers:
 *           named std::string values, a temporary string, and a moved-from variable.
 * - Act: Drain the ring back into std::string outputs.
 * - Assert: ShmSpscRing copies the payload bytes immediately, so all variants
 *           round-trip in order even though the shared-memory API exposes string_view.
 */
TEST_CASE(test_string_lvalue_and_temporary_pushes) {
    const std::string name = unique_name("string_inputs");
    auto ring = ShmSpscRing::create(name, 8, 64, true);
    REQUIRE_TRUE(ring.has_value());

    std::string alpha = "alpha";
    std::string beta = "beta";
    std::string delta = "delta";

    REQUIRE_TRUE(ring->tryPush(alpha));
    REQUIRE_TRUE(ring->tryPush(beta));
    REQUIRE_TRUE(ring->tryPush(std::string("gamma")));
    REQUIRE_TRUE(ring->tryPush(std::move(delta)));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(4));

    std::string value;
    const std::vector<std::string> expected{"alpha", "beta", "gamma", "delta"};
    for (const auto& exp : expected) {
        REQUIRE_TRUE(ring->tryPop(value));
        EXPECT_EQ(value, exp);
    }
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(0));
}

/**
 * Test: string_and_byte_payload_roundtrip
 * - Arrange: Push one message through the string overload and one through the
 *           byte-span overload.
 * - Act: Pop the first as a string and the second as raw bytes.
 * - Assert: Both insertion forms round-trip correctly through shared memory.
 */
TEST_CASE(test_string_and_byte_payload_roundtrip) {
    const std::string name = unique_name("payload_forms");
    auto ring = ShmSpscRing::create(name, 8, 64, true);
    REQUIRE_TRUE(ring.has_value());

    const std::string alpha = "alpha";
    const auto betaBytes = bytes_from_string("beta");

    REQUIRE_TRUE(ring->tryPush(alpha));
    REQUIRE_TRUE(ring->tryPush(std::span<const std::byte>(betaBytes.data(), betaBytes.size())));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(2));

    std::string textOut;
    REQUIRE_TRUE(ring->tryPop(textOut));
    EXPECT_EQ(textOut, alpha);

    std::vector<std::byte> bytesOut;
    REQUIRE_TRUE(ring->tryPop(bytesOut));
    EXPECT_EQ(string_from_bytes(bytesOut), std::string("beta"));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(0));
}

/**
 * Test: rejects_payload_larger_than_slot
 * - Arrange: Create a ring with a small per-slot payload limit.
 * - Act: Attempt to push a larger message.
 * - Assert: Oversized payloads are rejected and the ring remains empty.
 */
TEST_CASE(test_rejects_payload_larger_than_slot) {
    const std::string name = unique_name("oversize");
    auto ring = ShmSpscRing::create(name, 4, 16, true);
    REQUIRE_TRUE(ring.has_value());

    std::string payload(32, 'x');
    EXPECT_FALSE(ring->tryPush(payload));
    EXPECT_EQ(ring->size(), static_cast<std::size_t>(0));
}

/**
 * Test: requested_capacity_rounds_to_power_of_two_minus_one_usable_slots
 * - Arrange: Ask for a non-power-of-two capacity.
 * - Act: Create the ring.
 * - Assert: Internal slot count rounds up to a power of two, and one slot
 *           remains reserved so the usable capacity matches SpscRing semantics.
 */
TEST_CASE(test_requested_capacity_rounds_to_power_of_two_minus_one_usable_slots) {
    const std::string name = unique_name("capacity");
    auto ring = ShmSpscRing::create(name, 9, 64, true);
    REQUIRE_TRUE(ring.has_value());

    EXPECT_EQ(ring->capacity(), static_cast<std::size_t>(15));
    EXPECT_EQ(ring->maxPayloadBytes(), static_cast<std::size_t>(64));
}

int main() {
    run_test_case("test_create_open_roundtrip_and_capacity", test_create_open_roundtrip_and_capacity);
    run_test_case("test_push_until_full_then_reject", test_push_until_full_then_reject);
    run_test_case("test_repeated_pop_preserves_fifo_and_size_updates", test_repeated_pop_preserves_fifo_and_size_updates);
    run_test_case("test_wraparound_preserves_order_across_handles", test_wraparound_preserves_order_across_handles);
    run_test_case("test_overwrite_drops_oldest", test_overwrite_drops_oldest);
    run_test_case("test_string_lvalue_and_temporary_pushes", test_string_lvalue_and_temporary_pushes);
    run_test_case("test_string_and_byte_payload_roundtrip", test_string_and_byte_payload_roundtrip);
    run_test_case("test_rejects_payload_larger_than_slot", test_rejects_payload_larger_than_slot);
    run_test_case("test_requested_capacity_rounds_to_power_of_two_minus_one_usable_slots",
                  test_requested_capacity_rounds_to_power_of_two_minus_one_usable_slots);

    if (ctx.failed) {
        std::cerr << "\nTEST FAILURES: " << ctx.failed << " failed, " << ctx.passed << " passed\n";
        std::cerr << ctx.os.str();
        return 1;
    }

    std::cout << "All ShmSpscRing tests passed. Assertions: " << ctx.passed << "\n";
    return 0;
}
