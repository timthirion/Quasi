/// @file async_test.cpp
/// @brief Unit tests for the async module.

#include <quasi/async/async.hpp>

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace Q::async;

// ============================================================================
// task<T> tests
// ============================================================================

TEST_CASE("task<void> basic lifecycle", "[async][task]") {
    bool executed = false;

    auto coro = [&]() -> task<void> {
        executed = true;
        co_return;
    };

    SECTION("task is lazy - doesn't execute until resumed") {
        auto t = coro();
        REQUIRE_FALSE(executed);
        REQUIRE(t.valid());
        REQUIRE_FALSE(t.done());
    }

    SECTION("task executes when resumed") {
        auto t = coro();
        t.resume();
        REQUIRE(executed);
        REQUIRE(t.done());
    }
}

TEST_CASE("task<int> returns value", "[async][task]") {
    auto coro = []() -> task<int> {
        co_return 42;
    };

    auto t = coro();
    REQUIRE_FALSE(t.done());

    t.resume();
    REQUIRE(t.done());
    REQUIRE(t.result() == 42);
}

TEST_CASE("task move semantics", "[async][task]") {
    auto coro = []() -> task<int> {
        co_return 123;
    };

    auto t1 = coro();
    REQUIRE(t1.valid());

    auto t2 = std::move(t1);
    REQUIRE_FALSE(t1.valid());
    REQUIRE(t2.valid());

    t2.resume();
    REQUIRE(t2.result() == 123);
}

TEST_CASE("make_ready_task creates completed tasks", "[async][task]") {
    SECTION("void task") {
        auto t = make_ready_task();
        t.resume();
        REQUIRE(t.done());
    }

    SECTION("value task") {
        auto t = make_ready_task(99);
        t.resume();
        REQUIRE(t.done());
        REQUIRE(t.result() == 99);
    }
}

// ============================================================================
// scheduler tests
// ============================================================================

TEST_CASE("scheduler basic operation", "[async][scheduler]") {
    scheduler sched;

    REQUIRE(sched.empty());
    REQUIRE(sched.size() == 0);
    REQUIRE(sched.tick_count() == 0);
}

TEST_CASE("scheduler spawns and runs tasks", "[async][scheduler]") {
    scheduler sched;
    int counter = 0;

    auto coro = [&]() -> task<void> {
        counter++;
        co_return;
    };

    sched.spawn(coro());
    REQUIRE(sched.size() == 1);
    REQUIRE(counter == 0);

    sched.tick();
    REQUIRE(counter == 1);
    REQUIRE(sched.empty());
    REQUIRE(sched.tick_count() == 1);
}

TEST_CASE("scheduler runs multiple tasks", "[async][scheduler]") {
    scheduler sched;
    std::vector<int> order;

    auto make_coro = [&](int id) -> task<void> {
        order.push_back(id);
        co_return;
    };

    sched.spawn(make_coro(1));
    sched.spawn(make_coro(2));
    sched.spawn(make_coro(3));

    sched.run_until_empty();

    REQUIRE(order.size() == 3);
    REQUIRE(order[0] == 1);
    REQUIRE(order[1] == 2);
    REQUIRE(order[2] == 3);
}

// ============================================================================
// yield tests
// ============================================================================

TEST_CASE("yield suspends and re-enqueues", "[async][awaitables]") {
    scheduler sched;
    int stage = 0;

    auto coro = [&]() -> task<void> {
        stage = 1;
        co_await yield();
        stage = 2;
        co_await yield();
        stage = 3;
        co_return;
    };

    sched.spawn(coro());

    REQUIRE(stage == 0);

    sched.tick();
    REQUIRE(stage == 1);

    sched.tick();
    REQUIRE(stage == 2);

    sched.tick();
    REQUIRE(stage == 3);
    REQUIRE(sched.empty());
}

// ============================================================================
// wait_until tests
// ============================================================================

TEST_CASE("wait_until suspends until predicate is true", "[async][awaitables]") {
    scheduler sched;
    bool condition = false;
    bool completed = false;

    auto coro = [&]() -> task<void> {
        co_await wait_until([&] { return condition; });
        completed = true;
        co_return;
    };

    sched.spawn(coro());

    // Run a few ticks without setting condition
    sched.tick();
    sched.tick();
    REQUIRE_FALSE(completed);

    // Set condition and tick
    condition = true;
    sched.tick();
    REQUIRE(completed);
}

// ============================================================================
// file_watcher tests
// ============================================================================

TEST_CASE("file_watcher basic construction", "[async][file_watcher]") {
    file_watcher watcher{"/nonexistent/path/file.txt"};

    REQUIRE(watcher.path() == "/nonexistent/path/file.txt");
    REQUIRE_FALSE(watcher.exists());
    REQUIRE_FALSE(watcher.has_changed());
}

TEST_CASE("multi_file_watcher basic operations", "[async][file_watcher]") {
    multi_file_watcher watcher;

    REQUIRE(watcher.empty());
    REQUIRE(watcher.size() == 0);

    watcher.add("/path/one.txt");
    watcher.add("/path/two.txt");

    REQUIRE_FALSE(watcher.empty());
    REQUIRE(watcher.size() == 2);

    // No files exist, so no changes
    REQUIRE_FALSE(watcher.poll_change().has_value());
}
