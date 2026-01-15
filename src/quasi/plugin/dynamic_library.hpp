/// @file dynamic_library.hpp
/// @brief RAII wrapper for dynamic library loading (dlopen/dlclose).

#pragma once

#include <quasi/platform.hpp>

#include <dlfcn.h>

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace Q::plugin {

/// @brief Error codes for dynamic library operations.
enum class library_error {
    file_not_found,    ///< Library file does not exist.
    load_failed,       ///< dlopen() failed.
    symbol_not_found,  ///< dlsym() could not find the symbol.
    not_loaded,        ///< Operation requires a loaded library.
};

/// @brief Converts a library_error to a human-readable string.
/// @param err The error code.
/// @return String description of the error.
[[nodiscard]] constexpr std::string_view to_string(library_error err) noexcept {
    switch (err) {
        case library_error::file_not_found:   return "file not found";
        case library_error::load_failed:      return "failed to load library";
        case library_error::symbol_not_found: return "symbol not found";
        case library_error::not_loaded:       return "library not loaded";
    }
    return "unknown error";
}

/// @brief Returns the platform-appropriate shared library extension.
/// @return ".dylib" on macOS, ".so" on Linux, ".dll" on Windows.
[[nodiscard]] constexpr std::string_view shared_library_extension() noexcept {
    return Q_SHARED_LIB_EXTENSION;
}

/// @brief Ensures a path has the correct shared library extension.
/// @param path The path to check/modify.
/// @return The path with the appropriate extension added if missing.
[[nodiscard]] inline std::filesystem::path ensure_library_extension(
    const std::filesystem::path& path)
{
    auto ext = path.extension().string();

    if (ext == ".so" || ext == ".dylib" || ext == ".dll") {
        return path;
    }

    auto result = path;
    result += Q_SHARED_LIB_EXTENSION;
    return result;
}

/// @class dynamic_library
/// @brief RAII wrapper for dynamically loaded libraries.
///
/// Manages the lifetime of a shared library loaded via dlopen(). Automatically
/// closes the library when destroyed. Move-only to prevent double-close bugs.
class dynamic_library {
public:
    using handle_type = void*;
    using path_type   = std::filesystem::path;

    template <typename T>
    using result = std::expected<T, library_error>;

    dynamic_library() noexcept = default;

    ~dynamic_library() {
        close();
    }

    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator=(const dynamic_library&) = delete;

    dynamic_library(dynamic_library&& other) noexcept
        : handle_{std::exchange(other.handle_, nullptr)}
        , path_{std::move(other.path_)} {}

    dynamic_library& operator=(dynamic_library&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = std::exchange(other.handle_, nullptr);
            path_   = std::move(other.path_);
        }
        return *this;
    }

    /// @brief Opens a dynamic library.
    /// @param path Path to the library file.
    /// @return The loaded library, or an error.
    [[nodiscard]] static result<dynamic_library> open(const path_type& path) {
        if (!std::filesystem::exists(path)) {
            return std::unexpected{library_error::file_not_found};
        }

        dlerror();  // Clear previous error

        handle_type handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

        if (handle == nullptr) {
            return std::unexpected{library_error::load_failed};
        }

        dynamic_library lib;
        lib.handle_ = handle;
        lib.path_   = path;
        return lib;
    }

    /// @brief Looks up a symbol in the library.
    /// @tparam FuncPtr The function pointer type to cast to.
    /// @param name The symbol name to look up.
    /// @return The function pointer, or an error.
    template <typename FuncPtr>
    [[nodiscard]] result<FuncPtr> get_symbol(std::string_view name) const {
        if (!is_loaded()) {
            return std::unexpected{library_error::not_loaded};
        }

        dlerror();  // Clear previous error

        void* symbol = dlsym(handle_, name.data());

        if (const char* error = dlerror(); error != nullptr) {
            return std::unexpected{library_error::symbol_not_found};
        }

        return reinterpret_cast<FuncPtr>(symbol);
    }

    /// @brief Closes the library and releases resources.
    void close() noexcept {
        if (handle_ != nullptr) {
            dlclose(handle_);
            handle_ = nullptr;
            path_.clear();
        }
    }

    /// @brief Checks if a library is currently loaded.
    [[nodiscard]] bool is_loaded() const noexcept {
        return handle_ != nullptr;
    }

    /// @brief Boolean conversion for loaded state.
    [[nodiscard]] explicit operator bool() const noexcept {
        return is_loaded();
    }

    /// @brief Returns the path to the loaded library.
    [[nodiscard]] const path_type& path() const noexcept {
        return path_;
    }

    /// @brief Returns the native library handle.
    [[nodiscard]] handle_type native_handle() const noexcept {
        return handle_;
    }

    /// @brief Returns the last dlopen/dlsym error message.
    [[nodiscard]] static const char* last_error() noexcept {
        const char* err = dlerror();
        return err ? err : "no error";
    }

private:
    handle_type handle_ = nullptr;
    path_type   path_;
};

}  // namespace Q::plugin
