/// @file plugin.mm
/// @brief Simple Metal plugin that clears the screen with a cycling color.

#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/gpu/types.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cmath>

namespace {

/// @brief Plugin name and version.
constexpr const char* NAME        = "Metal Clear";
constexpr const char* DESCRIPTION = "Clears the screen with a cycling color";
constexpr const char* AUTHOR      = "Quasi";

/// @brief Internal plugin state.
struct plugin_state {
    Q_plugin_context* context = nullptr;
    float time = 0.0f;
};

}  // namespace

// Export symbols for dynamic loading
#define Q_EXPORT __attribute__((visibility("default")))

extern "C" {

Q_EXPORT uint32_t Q_plugin_abi_version(void) {
    return Q::plugin::k_plugin_abi_version;
}

Q_EXPORT Q_plugin_info Q_plugin_get_info(void) {
    return Q_plugin_info{
        .name        = NAME,
        .version     = {1, 0, 0},
        .description = DESCRIPTION,
        .author      = AUTHOR,
    };
}

Q_EXPORT Q_plugin_handle* Q_plugin_create(Q_plugin_context* ctx) {
    if (!ctx || !ctx->gpu) {
        return nullptr;
    }

    if (ctx->gpu->backend != Q_GPU_BACKEND_METAL) {
        if (ctx->log) {
            ctx->log(ctx->host_data, "Metal Clear plugin requires Metal backend");
        }
        return nullptr;
    }

    auto* state = new plugin_state{};
    state->context = ctx;

    if (ctx->log) {
        ctx->log(ctx->host_data, "Metal Clear plugin initialized");
    }

    return reinterpret_cast<Q_plugin_handle*>(state);
}

Q_EXPORT void Q_plugin_destroy(Q_plugin_handle* handle) {
    if (!handle) return;
    auto* state = reinterpret_cast<plugin_state*>(handle);

    if (state->context && state->context->log) {
        state->context->log(state->context->host_data, "Metal Clear plugin destroyed");
    }

    delete state;
}

Q_EXPORT void Q_plugin_update(Q_plugin_handle* handle, float delta_time) {
    if (!handle) return;
    auto* state = reinterpret_cast<plugin_state*>(handle);
    state->time += delta_time;
}

Q_EXPORT void Q_plugin_render(Q_plugin_handle* handle, Q_render_frame* frame) {
    if (!handle || !frame) return;

    auto* state = reinterpret_cast<plugin_state*>(handle);

    // Get Metal objects from frame
    id<CAMetalDrawable> drawable = (__bridge id<CAMetalDrawable>)frame->drawable;
    id<MTLCommandBuffer> command_buffer = (__bridge id<MTLCommandBuffer>)frame->command_buffer;

    if (!drawable || !command_buffer) return;

    // Create cycling color (HSV to RGB conversion)
    float hue = std::fmod(state->time * 0.2f, 1.0f);
    float saturation = 0.7f;
    float value = 0.9f;

    float c = value * saturation;
    float x = c * (1.0f - std::abs(std::fmod(hue * 6.0f, 2.0f) - 1.0f));
    float m = value - c;

    float r, g, b;
    int sector = static_cast<int>(hue * 6.0f) % 6;
    switch (sector) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    r += m; g += m; b += m;

    // Create render pass descriptor
    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    pass_desc.colorAttachments[0].texture = drawable.texture;
    pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(r, g, b, 1.0);

    // Create and end render encoder (just clears)
    id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
    [encoder endEncoding];
}

}  // extern "C"
