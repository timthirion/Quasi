/// @file exr_writer.hpp
/// @brief Writes HDR float data to OpenEXR files.

#pragma once

#include <quasi/plugin/plugin_interface.hpp>

#include <expected>
#include <filesystem>
#include <string>

namespace Q::io {

/// @brief Error codes for EXR write operations.
enum class exr_error {
    invalid_data,     ///< Null data or zero dimensions.
    write_failed,     ///< OpenEXR write operation failed.
    directory_error,  ///< Could not create output directory.
};

/// @brief Converts an exr_error to a human-readable string.
[[nodiscard]] constexpr const char* to_string(exr_error e) noexcept {
    switch (e) {
        case exr_error::invalid_data:    return "Invalid readback data";
        case exr_error::write_failed:    return "EXR write failed";
        case exr_error::directory_error: return "Could not create output directory";
    }
    return "Unknown error";
}

/// @brief Writes RGBA float data to an EXR file.
/// @param path Output file path.
/// @param result The readback data to write.
/// @return Success, or an error.
[[nodiscard]] std::expected<void, exr_error> write_exr(
    const std::filesystem::path& path,
    const Q_readback_result& result
);

/// @brief Writes all AOV buffers to a multi-layer EXR file.
/// @param path Output file path.
/// @param result The AOV readback data to write.
/// @return Success, or an error.
[[nodiscard]] std::expected<void, exr_error> write_exr(
    const std::filesystem::path& path,
    const Q_readback_aov_result& result
);

/// @brief Generates a timestamped filename like "quasi_20260326_153042.exr".
[[nodiscard]] std::filesystem::path make_timestamped_path(
    const std::filesystem::path& directory = "."
);

}  // namespace Q::io
