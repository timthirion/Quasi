/// @file types.hpp
/// @brief C ABI types for GPU context, agnostic to backend.
///
/// These types use opaque pointers to allow any GPU backend (Metal, Vulkan,
/// WebGPU) to provide its native handles through the same interface.

#pragma once

#include <cstdint>

extern "C" {

/// @brief Identifies which GPU backend is active.
enum Q_gpu_backend : uint32_t {
    Q_GPU_BACKEND_NONE   = 0,
    Q_GPU_BACKEND_METAL  = 1,
    Q_GPU_BACKEND_VULKAN = 2,
    Q_GPU_BACKEND_WEBGPU = 3,
};

/// @brief GPU device context passed to plugins at creation.
///
/// Contains long-lived GPU resources that persist across frames.
/// The actual types behind these pointers depend on Q_gpu_backend:
///
/// Metal:
///   - device: id<MTLDevice>
///   - queue:  id<MTLCommandQueue>
///
/// Vulkan:
///   - device: VkDevice
///   - queue:  VkQueue
///
/// WebGPU:
///   - device: WGPUDevice
///   - queue:  WGPUQueue
struct Q_gpu_context {
    Q_gpu_backend backend;  ///< Which GPU backend is active.
    void* device;           ///< GPU device handle.
    void* queue;            ///< Command queue/submission queue.
    void* layer;            ///< Swapchain/layer (CAMetalLayer, VkSwapchain, etc.)
};

/// @brief Per-frame render data passed to plugin render functions.
///
/// Contains resources valid only for the current frame.
///
/// Metal:
///   - drawable:           id<CAMetalDrawable>
///   - command_buffer:     id<MTLCommandBuffer>
///
/// Vulkan:
///   - drawable:           VkImage (swapchain image)
///   - command_buffer:     VkCommandBuffer
struct Q_render_frame {
    void* drawable;         ///< Current frame's drawable/swapchain image.
    void* command_buffer;   ///< Command buffer for this frame.
    uint32_t width;         ///< Drawable width in pixels.
    uint32_t height;        ///< Drawable height in pixels.
};

}  // extern "C"

namespace Q::gpu {

/// @brief C++ type aliases.
/// @{
using gpu_backend   = Q_gpu_backend;
using gpu_context   = Q_gpu_context;
using render_frame  = Q_render_frame;
/// @}

/// @brief Backend constants.
inline constexpr gpu_backend k_backend_none   = Q_GPU_BACKEND_NONE;
inline constexpr gpu_backend k_backend_metal  = Q_GPU_BACKEND_METAL;
inline constexpr gpu_backend k_backend_vulkan = Q_GPU_BACKEND_VULKAN;
inline constexpr gpu_backend k_backend_webgpu = Q_GPU_BACKEND_WEBGPU;

}  // namespace Q::gpu
