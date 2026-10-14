#include "neuron/container/pool.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <thread>
#include <vector>

namespace {

struct SimpleItem {
    int value{0};
};

struct ClearableItem {
    int value{0};
    bool cleared{false};
    void clear() {
        value = 0;
        cleared = true;
    }
};

}  // namespace

TEST_CASE("Pool: basic single-object alloc and free", "[Pool]") {
    Pool<SimpleItem> pool(4);

    SECTION("alloc returns non-null pointers") {
        SimpleItem* a = pool.alloc();
        REQUIRE(a != nullptr);
        a->value = 42;
        REQUIRE(a->value == 42);
    }

    SECTION("alloc returns distinct pointers") {
        SimpleItem* a = pool.alloc();
        SimpleItem* b = pool.alloc();
        REQUIRE(a != b);
    }

    SECTION("nget tracks outstanding allocations") {
        REQUIRE(pool.nget() == 0);
        SimpleItem* a = pool.alloc();
        REQUIRE(pool.nget() == 1);
        SimpleItem* b = pool.alloc();
        REQUIRE(pool.nget() == 2);
        pool.hpfree(a);
        REQUIRE(pool.nget() == 1);
        pool.hpfree(b);
        REQUIRE(pool.nget() == 0);
    }
}

TEST_CASE("Pool: automatic growth", "[Pool]") {
    Pool<SimpleItem> pool(2);

    SECTION("pool grows when exhausted") {
        // Initial capacity is 2
        SimpleItem* a = pool.alloc();
        SimpleItem* b = pool.alloc();
        // This should trigger growth
        SimpleItem* c = pool.alloc();
        REQUIRE(c != nullptr);
        REQUIRE(c != a);
        REQUIRE(c != b);
        REQUIRE(pool.nget() == 3);
    }

    SECTION("many allocations work correctly") {
        std::set<SimpleItem*> ptrs;
        for (int i = 0; i < 100; ++i) {
            SimpleItem* p = pool.alloc();
            REQUIRE(p != nullptr);
            // All pointers should be unique
            REQUIRE(ptrs.insert(p).second);
        }
        REQUIRE(pool.nget() == 100);
    }
}

TEST_CASE("Pool: free_all resets pool", "[Pool]") {
    Pool<SimpleItem> pool(8);

    SimpleItem* a = pool.alloc();
    SimpleItem* b = pool.alloc();
    SimpleItem* c = pool.alloc();
    (void) a;
    (void) b;
    (void) c;

    REQUIRE(pool.nget() == 3);
    pool.free_all();
    REQUIRE(pool.nget() == 0);

    // Can allocate again after free_all
    SimpleItem* d = pool.alloc();
    REQUIRE(d != nullptr);
    REQUIRE(pool.nget() == 1);
}

TEST_CASE("Pool: free_all calls clear() when available", "[Pool]") {
    Pool<ClearableItem> pool(4);

    ClearableItem* a = pool.alloc();
    a->value = 99;
    a->cleared = false;

    ClearableItem* b = pool.alloc();
    b->value = 77;
    b->cleared = false;

    pool.free_all();

    // After free_all, items should have been cleared
    REQUIRE(a->cleared == true);
    REQUIRE(a->value == 0);
    REQUIRE(b->cleared == true);
    REQUIRE(b->value == 0);
}

TEST_CASE("Pool: free_all does not call clear() when not available", "[Pool]") {
    // This just verifies it compiles and runs without error for types without clear()
    Pool<SimpleItem> pool(4);
    SimpleItem* a = pool.alloc();
    a->value = 42;
    pool.free_all();
    // SimpleItem has no clear(), so value is unchanged
    REQUIRE(a->value == 42);
}

TEST_CASE("Pool: is_valid_ptr", "[Pool]") {
    Pool<SimpleItem> pool(4);

    SimpleItem* a = pool.alloc();
    SimpleItem* b = pool.alloc();

    SECTION("valid pointers are recognized") {
        REQUIRE(pool.is_valid_ptr(a) == 1);
        REQUIRE(pool.is_valid_ptr(b) == 1);
    }

    SECTION("null pointer is not valid") {
        REQUIRE(pool.is_valid_ptr(nullptr) == 0);
    }

    SECTION("arbitrary pointer is not valid") {
        SimpleItem stack_item;
        REQUIRE(pool.is_valid_ptr(&stack_item) == 0);
    }

    SECTION("misaligned pointer is not valid") {
        char* misaligned = reinterpret_cast<char*>(a) + 1;
        REQUIRE(pool.is_valid_ptr(misaligned) == 0);
    }

    SECTION("valid after growth") {
        // Exhaust initial pool to force growth
        SimpleItem* c = pool.alloc();
        SimpleItem* d = pool.alloc();
        SimpleItem* e = pool.alloc();  // triggers growth
        REQUIRE(pool.is_valid_ptr(a) == 1);
        REQUIRE(pool.is_valid_ptr(b) == 1);
        REQUIRE(pool.is_valid_ptr(c) == 1);
        REQUIRE(pool.is_valid_ptr(d) == 1);
        REQUIRE(pool.is_valid_ptr(e) == 1);
    }
}

TEST_CASE("Pool: array pool (subcount > 1)", "[Pool]") {
    constexpr long subcount = 8;
    Pool<double> pool(4, subcount);

    SECTION("subcount() returns subcount") {
        REQUIRE(pool.subcount() == subcount);
    }

    SECTION("alloc returns arrays of subcount elements") {
        double* arr = pool.alloc();
        REQUIRE(arr != nullptr);
        // Should be able to write subcount elements without issue
        for (long i = 0; i < subcount; ++i) {
            arr[i] = static_cast<double>(i);
        }
        for (long i = 0; i < subcount; ++i) {
            REQUIRE(arr[i] == static_cast<double>(i));
        }
    }

    SECTION("multiple array allocations are non-overlapping") {
        double* a = pool.alloc();
        double* b = pool.alloc();
        // Pointers should be subcount elements apart (or more)
        REQUIRE(std::abs(b - a) >= subcount);
    }

    SECTION("total_allocs counts total allocations") {
        REQUIRE(pool.total_allocs() == 0);
        pool.alloc();
        REQUIRE(pool.total_allocs() == 1);
        pool.alloc();
        REQUIRE(pool.total_allocs() == 2);
        double* p = pool.alloc();
        REQUIRE(pool.total_allocs() == 3);
        pool.hpfree(p);
        // total_allocs does not decrease on free
        REQUIRE(pool.total_allocs() == 3);
    }

    SECTION("grow increases capacity") {
        // Allocate all 4 initial slots
        pool.alloc();
        pool.alloc();
        pool.alloc();
        pool.alloc();
        REQUIRE(pool.nget() == 4);
        // Free all and grow
        pool.free_all();
        pool.grow(8);
        REQUIRE(pool.size() == 12);  // 4 + 8
        // Can allocate up to the new capacity
        for (int i = 0; i < 12; ++i) {
            REQUIRE(pool.alloc() != nullptr);
        }
        REQUIRE(pool.nget() == 12);
    }

    SECTION("free_all resets array pool") {
        pool.alloc();
        pool.alloc();
        pool.alloc();
        REQUIRE(pool.nget() == 3);
        pool.free_all();
        REQUIRE(pool.nget() == 0);
    }
}

TEST_CASE("Pool: array pool growth via exhaustion", "[Pool]") {
    Pool<int> pool(2, 4);

    int* a = pool.alloc();
    int* b = pool.alloc();
    // Pool is now full (2 slots), next alloc triggers growth
    int* c = pool.alloc();
    REQUIRE(c != nullptr);
    REQUIRE(c != a);
    REQUIRE(c != b);

    // All still writable
    for (int i = 0; i < 4; ++i) {
        a[i] = i;
        b[i] = i + 10;
        c[i] = i + 20;
    }
    REQUIRE(a[3] == 3);
    REQUIRE(b[3] == 13);
    REQUIRE(c[3] == 23);
}

#if NRN_ENABLE_THREADS
TEST_CASE("Pool: mutex-protected pool is thread-safe", "[Pool]") {
    Pool<SimpleItem, true> pool(16);
    constexpr int per_thread = 200;
    constexpr int num_threads = 4;

    auto worker = [&pool]() {
        std::vector<SimpleItem*> ptrs;
        ptrs.reserve(per_thread);
        for (int i = 0; i < per_thread; ++i) {
            SimpleItem* p = pool.alloc();
            p->value = i;
            ptrs.push_back(p);
        }
        for (auto* p: ptrs) {
            pool.hpfree(p);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t: threads) {
        t.join();
    }

    // All items should be returned
    REQUIRE(pool.nget() == 0);
}
#endif
