/// @file exr_writer.cpp
/// @brief OpenEXR writer implementation.

#include <quasi/io/exr_writer.hpp>

#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <half.h>

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

std::expected<void, exr_error> write_exr(
    const std::filesystem::path& path,
    const Q_readback_aov_result& result
) {
    const auto& beauty = result.buffers[Q_AOV_BEAUTY];
    if (!beauty.data || beauty.width == 0 || beauty.height == 0) {
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

    uint32_t w = beauty.width;
    uint32_t h = beauty.height;
    size_t pixel_count = w * h;

    bool has_albedo = result.buffers[Q_AOV_ALBEDO].data != nullptr;
    bool has_normal = result.buffers[Q_AOV_NORMAL].data != nullptr;
    bool has_depth  = result.buffers[Q_AOV_DEPTH].data != nullptr;

    // Convert float32 RGBA buffers to interleaved half arrays.
    auto to_half = [](const float* src, size_t count) {
        std::vector<half> out(count * 4);
        for (size_t i = 0; i < count * 4; ++i) {
            out[i] = half(src[i]);
        }
        return out;
    };

    auto beauty_half = to_half(beauty.data, pixel_count);
    std::vector<half> albedo_half, normal_half;
    if (has_albedo) albedo_half = to_half(result.buffers[Q_AOV_ALBEDO].data, pixel_count);
    if (has_normal) normal_half = to_half(result.buffers[Q_AOV_NORMAL].data, pixel_count);

    try {
        Imf::Header header(w, h);

        // Beauty channels (default layer).
        header.channels().insert("R", Imf::Channel(Imf::HALF));
        header.channels().insert("G", Imf::Channel(Imf::HALF));
        header.channels().insert("B", Imf::Channel(Imf::HALF));
        header.channels().insert("A", Imf::Channel(Imf::HALF));

        if (has_albedo) {
            header.channels().insert("albedo.R", Imf::Channel(Imf::HALF));
            header.channels().insert("albedo.G", Imf::Channel(Imf::HALF));
            header.channels().insert("albedo.B", Imf::Channel(Imf::HALF));
        }
        if (has_normal) {
            header.channels().insert("normal.X", Imf::Channel(Imf::HALF));
            header.channels().insert("normal.Y", Imf::Channel(Imf::HALF));
            header.channels().insert("normal.Z", Imf::Channel(Imf::HALF));
        }
        if (has_depth) {
            header.channels().insert("depth.Z", Imf::Channel(Imf::FLOAT));
        }

        Imf::OutputFile file(path.c_str(), header);
        Imf::FrameBuffer fb;

        // Stride for interleaved RGBA half data.
        size_t half_pixel_stride = 4 * sizeof(half);
        size_t half_row_stride   = w * half_pixel_stride;

        // Beauty slices.
        char* b = reinterpret_cast<char*>(beauty_half.data());
        fb.insert("R", Imf::Slice(Imf::HALF, b + 0 * sizeof(half), half_pixel_stride, half_row_stride));
        fb.insert("G", Imf::Slice(Imf::HALF, b + 1 * sizeof(half), half_pixel_stride, half_row_stride));
        fb.insert("B", Imf::Slice(Imf::HALF, b + 2 * sizeof(half), half_pixel_stride, half_row_stride));
        fb.insert("A", Imf::Slice(Imf::HALF, b + 3 * sizeof(half), half_pixel_stride, half_row_stride));

        // Albedo slices.
        if (has_albedo) {
            char* a = reinterpret_cast<char*>(albedo_half.data());
            fb.insert("albedo.R", Imf::Slice(Imf::HALF, a + 0 * sizeof(half), half_pixel_stride, half_row_stride));
            fb.insert("albedo.G", Imf::Slice(Imf::HALF, a + 1 * sizeof(half), half_pixel_stride, half_row_stride));
            fb.insert("albedo.B", Imf::Slice(Imf::HALF, a + 2 * sizeof(half), half_pixel_stride, half_row_stride));
        }

        // Normal slices.
        if (has_normal) {
            char* n = reinterpret_cast<char*>(normal_half.data());
            fb.insert("normal.X", Imf::Slice(Imf::HALF, n + 0 * sizeof(half), half_pixel_stride, half_row_stride));
            fb.insert("normal.Y", Imf::Slice(Imf::HALF, n + 1 * sizeof(half), half_pixel_stride, half_row_stride));
            fb.insert("normal.Z", Imf::Slice(Imf::HALF, n + 2 * sizeof(half), half_pixel_stride, half_row_stride));
        }

        // Depth slice (full float, read from R channel of RGBA data).
        if (has_depth) {
            char* d = reinterpret_cast<char*>(const_cast<float*>(result.buffers[Q_AOV_DEPTH].data));
            size_t float_pixel_stride = 4 * sizeof(float);
            size_t float_row_stride   = w * float_pixel_stride;
            fb.insert("depth.Z", Imf::Slice(Imf::FLOAT, d, float_pixel_stride, float_row_stride));
        }

        file.setFrameBuffer(fb);
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
