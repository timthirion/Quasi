/// @file exr_writer.cpp
/// @brief OpenEXR writer implementation.

#include <quasi/io/exr_writer.hpp>

#include <ImfRgbaFile.h>
#include <ImfRgba.h>

#include <chrono>
#include <format>
#include <vector>

namespace Q::io {

std::expected<void, exr_error> write_exr(
    const std::filesystem::path& path,
    const Q_readback_result& result
) {
    if (!result.data || result.width == 0 || result.height == 0) {
        return std::unexpected{exr_error::invalid_data};
    }

    auto parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return std::unexpected{exr_error::directory_error};
        }
    }

    uint32_t w = result.width;
    uint32_t h = result.height;

    // Convert float32 RGBA to Imf::Rgba (half-float).
    std::vector<Imf::Rgba> pixels(w * h);
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            size_t src = (y * w + x) * 4;
            size_t dst = y * w + x;
            pixels[dst].r = result.data[src + 0];
            pixels[dst].g = result.data[src + 1];
            pixels[dst].b = result.data[src + 2];
            pixels[dst].a = result.data[src + 3];
        }
    }

    try {
        Imf::RgbaOutputFile file(path.c_str(), w, h, Imf::WRITE_RGBA);
        file.setFrameBuffer(pixels.data(), 1, w);
        file.writePixels(h);
    } catch (...) {
        return std::unexpected{exr_error::write_failed};
    }

    return {};
}

std::filesystem::path make_timestamped_path(const std::filesystem::path& directory) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&time, &local);

    auto filename = std::format("quasi_{:04}{:02}{:02}_{:02}{:02}{:02}.exr",
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec);

    return directory / filename;
}

}  // namespace Q::io
