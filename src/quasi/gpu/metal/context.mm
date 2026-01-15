/// @file context.mm
/// @brief Metal context implementation (Objective-C++).

#include <quasi/gpu/metal/context.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/NSWindow.h>
#import <AppKit/NSView.h>

namespace Q::gpu::metal {

context::~context() {
    release();
}

context::context(context&& other) noexcept
    : gpu_ctx_{other.gpu_ctx_}
    , width_{other.width_}
    , height_{other.height_}
{
    other.gpu_ctx_ = {};
    other.width_ = 0;
    other.height_ = 0;
}

context& context::operator=(context&& other) noexcept {
    if (this != &other) {
        release();
        gpu_ctx_ = other.gpu_ctx_;
        width_ = other.width_;
        height_ = other.height_;
        other.gpu_ctx_ = {};
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void context::release() {
    if (gpu_ctx_.queue) {
        CFRelease(gpu_ctx_.queue);
        gpu_ctx_.queue = nullptr;
    }
    if (gpu_ctx_.device) {
        CFRelease(gpu_ctx_.device);
        gpu_ctx_.device = nullptr;
    }
    // Layer is owned by the view, don't release it
    gpu_ctx_.layer = nullptr;
}

auto context::create(void* native_window) -> result<context> {
    if (!native_window) {
        return std::unexpected{context_error::layer_setup_failed};
    }

    NSWindow* window = (__bridge NSWindow*)native_window;
    NSView* content_view = [window contentView];
    if (!content_view) {
        return std::unexpected{context_error::layer_setup_failed};
    }

    // Create Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        return std::unexpected{context_error::device_not_found};
    }

    // Create command queue
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) {
        CFRelease((__bridge CFTypeRef)device);
        return std::unexpected{context_error::device_not_found};
    }

    // Create and configure Metal layer
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;

    // Set up the view's layer
    [content_view setWantsLayer:YES];
    [content_view setLayer:layer];

    CGSize size = content_view.bounds.size;
    CGFloat scale = window.backingScaleFactor;
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(size.width * scale, size.height * scale);

    context ctx;
    ctx.gpu_ctx_.backend = k_backend_metal;
    ctx.gpu_ctx_.device = (__bridge_retained void*)device;
    ctx.gpu_ctx_.queue = (__bridge_retained void*)queue;
    ctx.gpu_ctx_.layer = (__bridge void*)layer;  // Not retained, view owns it
    ctx.width_ = static_cast<uint32_t>(layer.drawableSize.width);
    ctx.height_ = static_cast<uint32_t>(layer.drawableSize.height);

    return ctx;
}

auto context::begin_frame() -> result<render_frame> {
    if (!gpu_ctx_.layer) {
        return std::unexpected{context_error::no_drawable};
    }

    CAMetalLayer* layer = (__bridge CAMetalLayer*)gpu_ctx_.layer;
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable) {
        return std::unexpected{context_error::no_drawable};
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)gpu_ctx_.queue;
    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    if (!command_buffer) {
        return std::unexpected{context_error::no_drawable};
    }

    render_frame frame{};
    frame.drawable = (__bridge_retained void*)drawable;
    frame.command_buffer = (__bridge_retained void*)command_buffer;
    frame.width = width_;
    frame.height = height_;

    return frame;
}

void context::end_frame(render_frame& frame) {
    if (!frame.drawable || !frame.command_buffer) {
        return;
    }

    id<MTLCommandBuffer> command_buffer = (__bridge_transfer id<MTLCommandBuffer>)frame.command_buffer;
    id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)frame.drawable;

    [command_buffer presentDrawable:drawable];
    [command_buffer commit];

    frame.drawable = nullptr;
    frame.command_buffer = nullptr;
}

void context::resize(uint32_t width, uint32_t height) {
    if (!gpu_ctx_.layer) {
        return;
    }

    CAMetalLayer* layer = (__bridge CAMetalLayer*)gpu_ctx_.layer;
    layer.drawableSize = CGSizeMake(width, height);
    width_ = width;
    height_ = height;
}

}  // namespace Q::gpu::metal
