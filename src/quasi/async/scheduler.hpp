/// @file scheduler.hpp
/// @brief Single-threaded cooperative coroutine scheduler.

#pragma once

#include <quasi/async/task.hpp>

#include <coroutine>
#include <cstdint>
#include <vector>

namespace Q::async {

class scheduler;

namespace detail {

/// @brief Thread-local pointer to the currently active scheduler.
inline thread_local scheduler* t_current_scheduler = nullptr;

}  // namespace detail

/// @brief Returns the scheduler running on the current thread.
/// @return Pointer to the current scheduler, or nullptr if none is active.
[[nodiscard]] inline scheduler* current_scheduler() noexcept {
    return detail::t_current_scheduler;
}

/// @class scheduler
/// @brief A single-threaded cooperative scheduler for coroutines.
///
/// The scheduler maintains a ready queue of coroutines and resumes them
/// one at a time during each tick(). Coroutines yield control back to the
/// scheduler using co_await yield() or similar awaitables.
///
/// This scheduler is non-preemptive: coroutines run until they explicitly
/// yield. It is also single-threaded: all coroutines run on the thread
/// that calls tick().
///
/// Example usage:
/// @code
/// scheduler sched;
/// sched.spawn(my_coroutine());
///
/// while (!sched.empty()) {
///     sched.tick();
///     // Do other per-frame work...
/// }
/// @endcode
class scheduler {
public:
    scheduler() = default;

    ~scheduler() {
        for (auto h : ready_queue_) {
            if (h && !h.done()) {
                h.destroy();
            }
        }
    }

    scheduler(const scheduler&) = delete;
    scheduler& operator=(const scheduler&) = delete;
    scheduler(scheduler&&) = delete;
    scheduler& operator=(scheduler&&) = delete;

    /// @brief Schedules a task to be driven by this scheduler.
    /// @param t The task to schedule. Ownership is transferred to the scheduler.
    void spawn(task<void> t) {
        if (t.valid() && !t.done()) {
            auto h = t.release();
            ready_queue_.push_back(h);
        }
    }

    /// @brief Enqueues a coroutine handle directly.
    /// @param h The coroutine handle to enqueue.
    /// @note Typically called by yield() to re-enqueue the current coroutine.
    void enqueue(std::coroutine_handle<> h) {
        if (h && !h.done()) {
            ready_queue_.push_back(h);
        }
    }

    /// @brief Runs one scheduler tick, resuming all ready coroutines.
    ///
    /// Each coroutine in the ready queue is resumed once. Coroutines that
    /// yield will re-enqueue themselves. Completed coroutines are destroyed.
    void tick() {
        ++tick_count_;

        auto* prev_scheduler = detail::t_current_scheduler;
        detail::t_current_scheduler = this;

        std::vector<std::coroutine_handle<>> to_run;
        std::swap(to_run, ready_queue_);

        for (auto h : to_run) {
            if (!h || h.done()) {
                if (h) {
                    h.destroy();
                }
                continue;
            }

            h.resume();

            if (h.done()) {
                h.destroy();
            }
        }

        detail::t_current_scheduler = prev_scheduler;
    }

    /// @brief Runs ticks until all coroutines complete.
    void run_until_empty() {
        while (!empty()) {
            tick();
        }
    }

    /// @brief Checks if the ready queue is empty.
    [[nodiscard]] bool empty() const noexcept {
        return ready_queue_.empty();
    }

    /// @brief Returns the number of coroutines in the ready queue.
    [[nodiscard]] std::size_t size() const noexcept {
        return ready_queue_.size();
    }

    /// @brief Returns the total number of ticks executed.
    [[nodiscard]] uint64_t tick_count() const noexcept {
        return tick_count_;
    }

private:
    std::vector<std::coroutine_handle<>> ready_queue_;
    uint64_t                             tick_count_ = 0;
};

/// @brief Returns a global default scheduler instance.
/// @note Use sparingly; prefer explicit scheduler instances for clarity.
inline scheduler& default_scheduler() {
    static scheduler instance;
    return instance;
}

/// @brief Spawns a task on the default scheduler.
/// @param t The task to spawn.
inline void spawn(task<void> t) {
    default_scheduler().spawn(std::move(t));
}

}  // namespace Q::async
