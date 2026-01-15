/// @file task.hpp
/// @brief Lazy, single-shot, movable coroutine type.

#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <variant>

namespace Q::async {

/// @class task
/// @brief A lazy coroutine that produces a value of type T.
///
/// Tasks are lazy: they don't start executing until awaited or manually resumed.
/// They are single-shot: once completed, they cannot be restarted.
/// They are move-only: copying would create ambiguous ownership of the coroutine frame.
///
/// @tparam T The type of value produced by the task. Defaults to void.
///
/// Example usage:
/// @code
/// task<int> compute_value() {
///     co_await some_async_operation();
///     co_return 42;
/// }
///
/// task<void> consumer() {
///     int value = co_await compute_value();
/// }
/// @endcode
template <typename T = void>
class task;

namespace detail {

/// @brief Promise type for task<T> where T is non-void.
template <typename T>
struct task_promise {
    using value_type  = T;
    using result_type = std::variant<std::monostate, T, std::exception_ptr>;

    result_type             result_;
    std::coroutine_handle<> continuation_;

    task<T> get_return_object() noexcept;

    /// @brief Lazy: suspend immediately at coroutine start.
    std::suspend_always initial_suspend() noexcept { return {}; }

    /// @brief At final suspend, transfer control to continuation if present.
    auto final_suspend() noexcept {
        struct final_awaiter {
            std::coroutine_handle<> cont;

            bool await_ready() noexcept { return false; }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                return cont ? cont : std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };
        return final_awaiter{continuation_};
    }

    void unhandled_exception() noexcept {
        result_.template emplace<2>(std::current_exception());
    }

    template <typename U>
    requires std::convertible_to<U, T>
    void return_value(U&& value) {
        result_.template emplace<1>(std::forward<U>(value));
    }

    /// @brief Retrieves the result, rethrowing any stored exception.
    [[nodiscard]] T get_result() {
        if (result_.index() == 2) {
            std::rethrow_exception(std::get<2>(result_));
        }
        return std::move(std::get<1>(result_));
    }

    /// @brief Checks if a result (value or exception) is available.
    [[nodiscard]] bool has_result() const noexcept {
        return result_.index() != 0;
    }
};

/// @brief Promise specialization for task<void>.
template <>
struct task_promise<void> {
    std::exception_ptr      exception_;
    std::coroutine_handle<> continuation_;
    bool                    returned_ = false;

    task<void> get_return_object() noexcept;

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
        struct final_awaiter {
            std::coroutine_handle<> cont;

            bool await_ready() noexcept { return false; }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                return cont ? cont : std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };
        return final_awaiter{continuation_};
    }

    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
    }

    void return_void() noexcept {
        returned_ = true;
    }

    void get_result() {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }

    [[nodiscard]] bool has_result() const noexcept {
        return returned_ || exception_;
    }
};

}  // namespace detail

template <typename T>
class [[nodiscard]] task {
public:
    using promise_type = detail::task_promise<T>;
    using handle_type  = std::coroutine_handle<promise_type>;

    /// @brief Constructs an empty task.
    task() noexcept = default;

    /// @brief Constructs a task from a coroutine handle.
    /// @param h The coroutine handle to take ownership of.
    explicit task(handle_type h) noexcept : handle_{h} {}

    /// @brief Destroys the task and its coroutine frame.
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    /// @brief Move constructor.
    task(task&& other) noexcept
        : handle_{std::exchange(other.handle_, nullptr)} {}

    /// @brief Move assignment operator.
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;

    // Awaitable interface

    /// @brief Returns true if the task is already complete.
    [[nodiscard]] bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    /// @brief Suspends the caller and starts/resumes this task.
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        handle_.promise().continuation_ = caller;
        return handle_;
    }

    /// @brief Returns the task's result when resumed.
    T await_resume() {
        return handle_.promise().get_result();
    }

    // Manual control

    /// @brief Checks if this task holds a valid coroutine.
    [[nodiscard]] bool valid() const noexcept {
        return handle_ != nullptr;
    }

    /// @brief Checks if the coroutine has completed.
    [[nodiscard]] bool done() const noexcept {
        return !handle_ || handle_.done();
    }

    /// @brief Resumes the coroutine. Used by the scheduler.
    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    /// @brief Gets the result after completion.
    /// @throws Rethrows any exception stored in the coroutine.
    T result() {
        return handle_.promise().get_result();
    }

    /// @brief Releases ownership of the coroutine handle.
    /// @return The coroutine handle. Caller takes ownership.
    [[nodiscard]] handle_type release() noexcept {
        return std::exchange(handle_, nullptr);
    }

    /// @brief Returns the raw coroutine handle without releasing ownership.
    [[nodiscard]] handle_type handle() const noexcept {
        return handle_;
    }

private:
    handle_type handle_ = nullptr;
};

// Promise get_return_object implementations

template <typename T>
task<T> detail::task_promise<T>::get_return_object() noexcept {
    return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
}

inline task<void> detail::task_promise<void>::get_return_object() noexcept {
    return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}

/// @brief Creates an already-completed task with a value.
/// @param value The value to return from the task.
/// @return A task that immediately produces the given value.
template <typename T>
task<T> make_ready_task(T value) {
    co_return std::move(value);
}

/// @brief Creates an already-completed void task.
/// @return A task that immediately completes.
inline task<void> make_ready_task() {
    co_return;
}

}  // namespace Q::async
