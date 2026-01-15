/// @file manager.hpp
/// @brief Plugin manager with hot-reload support.

#pragma once

#include <quasi/plugin/dynamic_library.hpp>
#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/plugin/loader.hpp>
#include <quasi/async/async.hpp>

#include <atomic>
#include <chrono>
#include <expected>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace Q::plugin {

/// @brief Callbacks for plugin reload events.
///
/// Use these hooks to integrate with GPU resources that need special
/// handling during plugin reload (e.g., waiting for GPU idle).
struct reload_hooks {
    /// @brief Called before unloading the current plugin.
    /// Use this to wait for GPU idle, flush caches, etc.
    std::function<async::task<void>()> pre_unload =
        []() -> async::task<void> { co_return; };

    /// @brief Called after successfully loading a new plugin.
    /// Use this for warmup, validation, shader compilation, etc.
    std::function<async::task<void>()> post_load =
        []() -> async::task<void> { co_return; };

    /// @brief Called when a reload fails.
    /// @param message Description of the error.
    std::function<void(const std::string&)> on_error =
        [](const std::string&) {};
};

/// @brief Statistics about plugin reloads.
struct reload_stats {
    uint64_t reload_count      = 0;  ///< Total reload attempts.
    uint64_t success_count     = 0;  ///< Successful reloads.
    uint64_t failure_count     = 0;  ///< Failed reloads.
    float    last_reload_time  = 0.0f;  ///< Last reload duration in seconds.
};

/// @class manager
/// @brief Manages a hot-reloadable plugin with async file watching.
///
/// Watches the plugin library file for changes and automatically reloads
/// when modifications are detected. Integrates with the async scheduler
/// for non-blocking operation.
///
/// Example usage:
/// @code
/// manager mgr{"libbackend.dylib"};
/// mgr.set_viewport(1920, 1080);
///
/// if (!mgr.load_sync()) {
///     std::cerr << "Failed to load plugin\n";
///     return;
/// }
///
/// scheduler sched;
/// sched.spawn(mgr.watch_and_reload_loop());
///
/// while (running) {
///     sched.tick();
///     mgr.update(delta_time);
///     mgr.render();
/// }
/// @endcode
class manager {
public:
    using path_type = std::filesystem::path;

    /// @brief Error codes for plugin operations.
    enum class error {
        file_not_found,   ///< Plugin file does not exist.
        load_failed,      ///< Failed to load the library.
        abi_mismatch,     ///< ABI version mismatch.
        symbol_missing,   ///< Required symbol not found.
        create_failed,    ///< Plugin creation returned nullptr.
        already_loading,  ///< A load operation is already in progress.
    };

    template <typename T>
    using result = std::expected<T, error>;

    /// @brief Constructs a manager for the specified plugin library.
    /// @param library_path Path to the plugin shared library.
    /// @param hooks Optional reload event callbacks.
    explicit manager(path_type library_path, reload_hooks hooks = {})
        : library_path_{std::move(library_path)}
        , watcher_{library_path_}
        , hooks_{std::move(hooks)} {}

    ~manager() = default;

    manager(const manager&) = delete;
    manager& operator=(const manager&) = delete;
    manager(manager&&) = delete;
    manager& operator=(manager&&) = delete;

    /// @brief Performs initial synchronous plugin load.
    /// @return Success or an error code.
    [[nodiscard]] result<void> load_sync() {
        return do_load();
    }

    /// @brief Coroutine that watches for file changes and reloads.
    ///
    /// Spawn this on a scheduler to enable automatic hot-reloading.
    /// Runs indefinitely until the scheduler is stopped.
    [[nodiscard]] async::task<void> watch_and_reload_loop() {
        std::cout << "[plugin::manager] Watching: " << library_path_ << "\n";

        while (true) {
            co_await async::wait_ms(100);  // Throttle filesystem polling

            if (!watcher_.has_changed()) {
                continue;
            }

            std::cout << "\n[plugin::manager] File changed: " << library_path_ << "\n";
            co_await do_reload_async();
        }
    }

    /// @brief Triggers an async reload manually.
    /// @return A task that completes when the reload finishes.
    [[nodiscard]] async::task<result<void>> reload_async() {
        co_return co_await do_reload_async();
    }

    /// @brief Calls the plugin's update function.
    /// @param delta_time Seconds since the last update.
    void update(float delta_time) {
        if (plugin_) {
            plugin_->update(delta_time);
        }
    }

    /// @brief Calls the plugin's render function.
    /// @param frame Per-frame render data (drawable, command buffer, etc.)
    void render(Q::gpu::render_frame* frame) {
        if (plugin_) {
            plugin_->render(frame);
        }
    }

    /// @brief Sets the viewport dimensions in the plugin context.
    /// @param width Viewport width in pixels.
    /// @param height Viewport height in pixels.
    void set_viewport(uint32_t width, uint32_t height) {
        context_.viewport_width  = width;
        context_.viewport_height = height;
    }

    /// @brief Sets the host data pointer in the plugin context.
    /// @param data Pointer to host-specific data.
    void set_host_data(void* data) {
        context_.host_data = data;
    }

    /// @brief Sets the GPU context in the plugin context.
    /// @param gpu GPU context pointer.
    void set_gpu_context(Q::gpu::gpu_context* gpu) {
        context_.gpu = gpu;
    }

    /// @brief Sets the logging callback in the plugin context.
    /// @param fn Logging function pointer.
    void set_log_callback(void (*fn)(void*, const char*)) {
        context_.log = fn;
    }

    /// @brief Checks if a plugin is currently loaded and valid.
    [[nodiscard]] bool is_loaded() const noexcept {
        return plugin_.has_value() && plugin_->is_valid();
    }

    /// @brief Returns the loaded plugin's metadata.
    [[nodiscard]] std::optional<plugin_info> info() const {
        if (plugin_) {
            return plugin_->info();
        }
        return std::nullopt;
    }

    /// @brief Returns reload statistics.
    [[nodiscard]] const reload_stats& stats() const noexcept {
        return stats_;
    }

    /// @brief Returns the path to the plugin library.
    [[nodiscard]] const path_type& library_path() const noexcept {
        return library_path_;
    }

private:
    async::task<result<void>> do_reload_async() {
        using clock = std::chrono::steady_clock;
        auto start_time = clock::now();

        ++stats_.reload_count;

        std::cout << "[plugin::manager] Starting reload...\n";

        // Run pre-unload hook
        std::cout << "[plugin::manager] Pre-unload hook...\n";
        {
            auto task = hooks_.pre_unload();
            while (!task.done()) {
                task.resume();
            }
        }

        unload_current();

        // Wait for filesystem to settle
        std::cout << "[plugin::manager] Waiting for filesystem...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds{300});

        // Load new plugin
        std::cout << "[plugin::manager] Loading new library...\n";
        result<void> load_result;
        try {
            load_result = do_load();
        } catch (const std::exception& e) {
            std::cerr << "[plugin::manager] Exception: " << e.what() << "\n";
            load_result = std::unexpected{error::load_failed};
        }

        if (!load_result) {
            ++stats_.failure_count;
            hooks_.on_error("Load failed");
            std::cerr << "[plugin::manager] Reload failed!\n";
            co_return std::unexpected{load_result.error()};
        }

        // Run post-load hook
        std::cout << "[plugin::manager] Post-load hook...\n";
        {
            auto task = hooks_.post_load();
            while (!task.done()) {
                task.resume();
            }
        }

        ++stats_.success_count;
        auto elapsed = clock::now() - start_time;
        stats_.last_reload_time = std::chrono::duration<float>(elapsed).count();

        std::cout << "[plugin::manager] Reload complete in "
                  << (stats_.last_reload_time * 1000.0f) << " ms\n";

        watcher_.refresh_timestamp();

        co_return result<void>{};
    }

    result<void> do_load() {
        auto temp_path = make_temp_library_path();

        try {
            if (std::filesystem::exists(temp_path)) {
                std::filesystem::remove(temp_path);
            }
            std::filesystem::copy_file(
                library_path_,
                temp_path,
                std::filesystem::copy_options::overwrite_existing
            );
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[plugin::manager] Copy failed: " << e.what() << "\n";
            return std::unexpected{error::load_failed};
        }

        auto lib_result = dynamic_library::open(temp_path);
        if (!lib_result) {
            std::cerr << "[plugin::manager] dlopen failed: "
                      << dynamic_library::last_error() << "\n";
            return std::unexpected{error::load_failed};
        }

        library_ = std::move(*lib_result);

        auto loader_result = loader::load(library_, &context_);
        if (!loader_result) {
            std::cerr << "[plugin::manager] Plugin load failed: "
                      << to_string(loader_result.error()) << "\n";
            library_.close();

            switch (loader_result.error()) {
                case loader::error::abi_mismatch:
                    return std::unexpected{error::abi_mismatch};
                case loader::error::symbol_not_found:
                    return std::unexpected{error::symbol_missing};
                case loader::error::create_failed:
                    return std::unexpected{error::create_failed};
            }
            return std::unexpected{error::load_failed};
        }

        plugin_ = std::move(*loader_result);

        if (auto i = info()) {
            std::cout << "[plugin::manager] Loaded: " << i->name
                      << " v" << i->version.major
                      << "." << i->version.minor
                      << "." << i->version.patch << "\n";
        }

        watcher_.refresh_timestamp();
        return {};
    }

    [[nodiscard]] path_type make_temp_library_path() const {
        auto filename = library_path_.filename().string();
        auto temp_dir = std::filesystem::temp_directory_path();

        static std::atomic<uint64_t> counter{0};
        auto unique_name = "quasi_plugin_" + std::to_string(counter++) + "_" + filename;

        return temp_dir / unique_name;
    }

    void unload_current() {
        if (plugin_) {
            std::cout << "[plugin::manager] Destroying plugin...\n";
            plugin_->destroy();
            plugin_.reset();
        }
        if (library_.is_loaded()) {
            library_.close();
            std::cout << "[plugin::manager] Library unloaded.\n";
        }
    }

    path_type               library_path_;
    dynamic_library         library_;
    async::file_watcher     watcher_;
    reload_hooks            hooks_;
    reload_stats            stats_;
    plugin_context          context_{};
    std::optional<loader>   plugin_;
};

/// @brief Converts a manager error to a human-readable string.
[[nodiscard]] inline constexpr std::string_view to_string(manager::error e) noexcept {
    switch (e) {
        case manager::error::file_not_found:  return "file not found";
        case manager::error::load_failed:     return "load failed";
        case manager::error::abi_mismatch:    return "ABI version mismatch";
        case manager::error::symbol_missing:  return "missing symbols";
        case manager::error::create_failed:   return "plugin creation failed";
        case manager::error::already_loading: return "already loading";
    }
    return "unknown error";
}

}  // namespace Q::plugin
