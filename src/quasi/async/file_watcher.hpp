/// @file file_watcher.hpp
/// @brief Filesystem change detection for hot-reloading.

#pragma once

#include <quasi/async/task.hpp>
#include <quasi/async/awaitables.hpp>

#include <chrono>
#include <filesystem>
#include <optional>
#include <vector>

namespace Q::async {

/// @brief Information about a detected file change.
struct file_change {
    std::filesystem::path           path;      ///< Path to the changed file.
    std::filesystem::file_time_type old_time;  ///< Previous modification time.
    std::filesystem::file_time_type new_time;  ///< New modification time.
};

/// @class file_watcher
/// @brief Watches a single file for modifications.
///
/// Uses filesystem timestamps to detect changes. Call has_changed() to poll
/// for modifications, or use the async wait_for_change() method.
class file_watcher {
public:
    using path_type = std::filesystem::path;
    using time_type = std::filesystem::file_time_type;

    /// @brief Constructs a watcher for the specified file.
    /// @param path Path to the file to watch.
    explicit file_watcher(path_type path)
        : path_{std::move(path)} {
        refresh_timestamp();
    }

    /// @brief Checks if the file has been modified since the last check.
    /// @return True if the file's timestamp has changed.
    [[nodiscard]] bool has_changed() const {
        if (!std::filesystem::exists(path_)) {
            return false;
        }

        try {
            auto current_time = std::filesystem::last_write_time(path_);
            return last_write_time_.has_value() &&
                   current_time != *last_write_time_;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    /// @brief Updates the stored timestamp to the file's current modification time.
    void refresh_timestamp() {
        if (std::filesystem::exists(path_)) {
            try {
                last_write_time_ = std::filesystem::last_write_time(path_);
            } catch (const std::filesystem::filesystem_error&) {
                last_write_time_.reset();
            }
        } else {
            last_write_time_.reset();
        }
    }

    /// @brief Returns an awaitable that suspends until the file changes.
    /// @return An awaitable that produces a file_change when the file is modified.
    [[nodiscard]] auto wait_for_change() {
        struct change_awaiter {
            file_watcher* watcher;

            [[nodiscard]] bool await_ready() const {
                return watcher->has_changed();
            }

            [[nodiscard]] bool await_suspend(std::coroutine_handle<>) const {
                return !watcher->has_changed();
            }

            [[nodiscard]] file_change await_resume() const {
                auto old_time = watcher->last_write_time_.value_or(time_type{});
                auto new_time = std::filesystem::last_write_time(watcher->path_);
                watcher->refresh_timestamp();

                return {
                    .path     = watcher->path_,
                    .old_time = old_time,
                    .new_time = new_time,
                };
            }
        };

        return change_awaiter{this};
    }

    /// @brief Asynchronously waits for the next file change.
    /// @return A task that produces a file_change when modification is detected.
    [[nodiscard]] task<file_change> next_change() {
        co_await wait_until([this] { return has_changed(); });

        auto old_time = last_write_time_.value_or(time_type{});
        auto new_time = std::filesystem::last_write_time(path_);
        refresh_timestamp();

        co_return file_change{
            .path     = path_,
            .old_time = old_time,
            .new_time = new_time,
        };
    }

    /// @brief Returns the watched file path.
    [[nodiscard]] const path_type& path() const noexcept {
        return path_;
    }

    /// @brief Returns the last recorded modification time.
    [[nodiscard]] std::optional<time_type> last_write_time() const noexcept {
        return last_write_time_;
    }

    /// @brief Checks if the watched file exists.
    [[nodiscard]] bool exists() const {
        return std::filesystem::exists(path_);
    }

private:
    path_type                path_;
    std::optional<time_type> last_write_time_;
};

/// @class multi_file_watcher
/// @brief Watches multiple files for modifications.
class multi_file_watcher {
public:
    using path_type = std::filesystem::path;

    /// @brief Adds a file to the watch list.
    /// @param path Path to the file to watch.
    void add(path_type path) {
        watchers_.emplace_back(std::move(path));
    }

    /// @brief Adds multiple files to the watch list.
    /// @param paths Paths to watch.
    void add(std::initializer_list<path_type> paths) {
        for (const auto& p : paths) {
            add(p);
        }
    }

    /// @brief Polls for a change in any watched file.
    /// @return The first detected change, or nullopt if no changes.
    [[nodiscard]] std::optional<file_change> poll_change() {
        for (auto& w : watchers_) {
            if (w.has_changed()) {
                auto old_time = w.last_write_time().value_or(file_watcher::time_type{});
                auto new_time = std::filesystem::last_write_time(w.path());
                w.refresh_timestamp();

                return file_change{
                    .path     = w.path(),
                    .old_time = old_time,
                    .new_time = new_time,
                };
            }
        }
        return std::nullopt;
    }

    /// @brief Returns an awaitable that suspends until any file changes.
    [[nodiscard]] auto wait_for_any_change() {
        struct any_change_awaiter {
            multi_file_watcher* watcher;

            [[nodiscard]] bool await_ready() const {
                return watcher->poll_change().has_value();
            }

            [[nodiscard]] bool await_suspend(std::coroutine_handle<>) const {
                return !watcher->poll_change().has_value();
            }

            [[nodiscard]] file_change await_resume() const {
                return *watcher->poll_change();
            }
        };

        return any_change_awaiter{this};
    }

    /// @brief Refreshes timestamps for all watched files.
    void refresh_all() {
        for (auto& w : watchers_) {
            w.refresh_timestamp();
        }
    }

    /// @brief Returns the number of watched files.
    [[nodiscard]] std::size_t size() const noexcept {
        return watchers_.size();
    }

    /// @brief Checks if no files are being watched.
    [[nodiscard]] bool empty() const noexcept {
        return watchers_.empty();
    }

private:
    std::vector<file_watcher> watchers_;
};

}  // namespace Q::async
