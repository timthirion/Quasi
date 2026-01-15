/// @file context.mm
/// @brief Metal context implementation (Objective-C++).

#include <quasi/gpu/metal/context.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <AppKit/NSWindow.h>
#import <AppKit/NSView.h>

#include <dispatch/dispatch.h>

namespace Q::gpu::metal {

namespace {
constexpr int k_max_frames_in_flight = 3;
}

context::~context() {
    release();
}

context::context(context&& other) noexcept
    : gpu_ctx_{other.gpu_ctx_}
    , frame_semaphore_{other.frame_semaphore_}
    , width_{other.width_}
    , height_{other.height_}
{
    other.gpu_ctx_ = {};
    other.frame_semaphore_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

context& context::operator=(context&& other) noexcept {
    if (this != &other) {
        release();
        gpu_ctx_ = other.gpu_ctx_;
        frame_semaphore_ = other.frame_semaphore_;
        width_ = other.width_;
        height_ = other.height_;
        other.gpu_ctx_ = {};
        other.frame_semaphore_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

void context::release() {
    // Wait for all in-flight frames to complete before releasing resources.
    if (frame_semaphore_) {
        dispatch_semaphore_t sem = (__bridge dispatch_semaphore_t)frame_semaphore_;
        for (int i = 0; i < k_max_frames_in_flight; i++) {
            dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
        }
        for (int i = 0; i < k_max_frames_in_flight; i++) {
            dispatch_semaphore_signal(sem);
        }
        // Release our retain from create().
        CFRelease(frame_semaphore_);
        frame_semaphore_ = nullptr;
    }
    if (gpu_ctx_.queue) {
        CFRelease(gpu_ctx_.queue);
        gpu_ctx_.queue = nullptr;
    }
    if (gpu_ctx_.device) {
        CFRelease(gpu_ctx_.device);
        gpu_ctx_.device = nullptr;
    }
    // Layer is owned by the view, don't release it.
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

    // Create Metal device.
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        return std::unexpected{context_error::device_not_found};
    }

    // Create command queue.
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) {
        CFRelease((__bridge CFTypeRef)device);
        return std::unexpected{context_error::device_not_found};
    }

    // Create and configure Metal layer.
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    layer.maximumDrawableCount = k_max_frames_in_flight;

    // Set up the view's layer.
    [content_view setWantsLayer:YES];
    [content_view setLayer:layer];

    CGSize size = content_view.bounds.size;
    CGFloat scale = window.backingScaleFactor;
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(size.width * scale, size.height * scale);

    // Create semaphore for triple buffering.
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(k_max_frames_in_flight);

    context ctx;
    ctx.gpu_ctx_.backend = k_backend_metal;
    ctx.gpu_ctx_.device = (__bridge_retained void*)device;
    ctx.gpu_ctx_.queue = (__bridge_retained void*)queue;
    ctx.gpu_ctx_.layer = (__bridge void*)layer;  // Not retained, view owns it.
    ctx.frame_semaphore_ = (__bridge_retained void*)semaphore;
    ctx.width_ = static_cast<uint32_t>(layer.drawableSize.width);
    ctx.height_ = static_cast<uint32_t>(layer.drawableSize.height);

    return ctx;
}

auto context::begin_frame() -> result<render_frame> {
    if (!gpu_ctx_.layer || !frame_semaphore_) {
        return std::unexpected{context_error::no_drawable};
    }

    // Wait for a frame slot to become available.
    dispatch_semaphore_t sem = (__bridge dispatch_semaphore_t)frame_semaphore_;
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

    CAMetalLayer* layer = (__bridge CAMetalLayer*)gpu_ctx_.layer;
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable) {
        dispatch_semaphore_signal(sem);  // Release the slot we just acquired.
        return std::unexpected{context_error::no_drawable};
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)gpu_ctx_.queue;
    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    if (!command_buffer) {
        dispatch_semaphore_signal(sem);
        return std::unexpected{context_error::no_drawable};
    }

    // Get actual dimensions from drawable texture.
    id<MTLTexture> texture = drawable.texture;

    render_frame frame{};
    frame.drawable = (__bridge_retained void*)drawable;
    frame.command_buffer = (__bridge_retained void*)command_buffer;
    frame.width = static_cast<uint32_t>(texture.width);
    frame.height = static_cast<uint32_t>(texture.height);

    return frame;
}

void context::end_frame(render_frame& frame) {
    if (!frame.drawable || !frame.command_buffer) {
        return;
    }

    id<MTLCommandBuffer> command_buffer = (__bridge_transfer id<MTLCommandBuffer>)frame.command_buffer;
    id<CAMetalDrawable> drawable = (__bridge_transfer id<CAMetalDrawable>)frame.drawable;

    [command_buffer presentDrawable:drawable];

    // Signal semaphore when GPU completes this frame.
    // Prevent capturing `this` - copy the pointer.
    void* sem_ptr = frame_semaphore_;
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
        dispatch_semaphore_t sem = (__bridge dispatch_semaphore_t)sem_ptr;
        dispatch_semaphore_signal(sem);
    }];

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
