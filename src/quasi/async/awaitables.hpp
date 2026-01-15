/// @file awaitables.hpp
/// @brief Common awaitable types for coroutine control flow.

#pragma once

#include <quasi/async/task.hpp>
#include <quasi/async/scheduler.hpp>

#include <chrono>
#include <concepts>
#include <coroutine>
#include <functional>

namespace Q::async {

/// @brief Awaitable that suspends for one scheduler tick.
///
/// When co_awaited, yields control back to the scheduler and re-enqueues
/// the current coroutine to be resumed on the next tick.
struct yield_awaitable {
    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) const noexcept {
        if (auto* sched = detail::t_current_scheduler) {
            sched->enqueue(h);
        }
    }

    void await_resume() const noexcept {}
};

/// @brief Suspends the current coroutine for one scheduler tick.
/// @return An awaitable that yields to the scheduler.
[[nodiscard]] inline yield_awaitable yield() noexcept {
    return {};
}

using std::suspend_always;
using std::suspend_never;

/// @brief Suspends until a predicate returns true.
/// @tparam Pred A callable that returns a bool-convertible value.
/// @param pred The predicate to check each tick.
/// @return A task that completes when pred() returns true.
template <std::invocable<> Pred>
requires std::convertible_to<std::invoke_result_t<Pred>, bool>
task<void> wait_until(Pred pred) {
    while (!std::invoke(pred)) {
        co_await yield();
    }
}

/// @brief Suspends for a specified duration.
/// @tparam Rep Duration representation type.
/// @tparam Period Duration period type.
/// @param duration How long to wait.
/// @return A task that completes after the duration elapses.
template <typename Rep, typename Period>
task<void> wait_for(std::chrono::duration<Rep, Period> duration) {
    auto end_time = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < end_time) {
        co_await yield();
    }
}

/// @brief Suspends for the specified number of milliseconds.
/// @param ms Milliseconds to wait.
inline task<void> wait_ms(int64_t ms) {
    return wait_for(std::chrono::milliseconds{ms});
}

/// @brief Suspends for the specified number of microseconds.
/// @param us Microseconds to wait.
inline task<void> wait_us(int64_t us) {
    return wait_for(std::chrono::microseconds{us});
}

/// @brief Waits for all provided tasks to complete.
/// @tparam Tasks Task types (must be task<T> or similar).
/// @param tasks The tasks to wait for.
/// @return A task that completes when all input tasks are done.
template <typename... Tasks>
task<void> when_all(Tasks... tasks) {
    auto all_done = [&]() {
        return (tasks.done() && ...);
    };

    while (!all_done()) {
        ((!tasks.done() ? (tasks.resume(), 0) : 0), ...);

        if (!all_done()) {
            co_await yield();
        }
    }
}

namespace detail {

template <std::size_t... Is, typename... Tasks>
task<std::size_t> when_any_impl(std::index_sequence<Is...>, Tasks&... tasks) {
    std::size_t completed_index = 0;
    bool found = false;

    auto check_one = [&]<std::size_t I>(std::integral_constant<std::size_t, I>, auto& t) {
        if (!found && t.done()) {
            completed_index = I;
            found = true;
        }
    };

    while (!found) {
        (check_one(std::integral_constant<std::size_t, Is>{}, tasks), ...);

        if (!found) {
            ((tasks.done() ? void() : tasks.resume()), ...);
            co_await yield();
        }
    }

    co_return completed_index;
}

}  // namespace detail

/// @brief Waits for any of the provided tasks to complete.
/// @tparam Tasks Task types.
/// @param tasks The tasks to race.
/// @return A task that produces the index of the first completed task.
template <typename... Tasks>
task<std::size_t> when_any(Tasks&... tasks) {
    return detail::when_any_impl(
        std::index_sequence_for<Tasks...>{},
        tasks...
    );
}

}  // namespace Q::async
