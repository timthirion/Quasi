/// @file plugin.mm
/// @brief Metal backend - Cornell Box path tracer.

#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/gpu/types.hpp>
#include <quasi/scene/cornell_box.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cmath>

namespace {

constexpr const char* NAME        = "Metal Path Tracer";
constexpr const char* DESCRIPTION = "Cornell Box path tracer using Metal";
constexpr const char* AUTHOR      = "Quasi";

constexpr uint32_t MAX_QUADS   = 32;
constexpr uint32_t MAX_BOUNCES = 5;
constexpr uint32_t SAMPLES_PER_FRAME = 1;

// Path tracing shader.
constexpr const char* k_shader_source = R"(
#include <metal_stdlib>
using namespace metal;

#define MAX_QUADS 32
#define MAX_BOUNCES 5

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

// Fullscreen triangle - no vertex buffer needed.
vertex VertexOut fullscreen_vert(uint vid [[vertex_id]]) {
    VertexOut out;
    out.uv = float2((vid << 1) & 2, vid & 2);
    out.position = float4(out.uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return out;
}

// ----- Data Structures -----

struct Camera {
    packed_float3 position;
    float fov;
    packed_float3 direction;
    float aspect;
    packed_float3 up;
    float _pad;
};

struct Quad {
    packed_float3 origin;
    float _p0;
    packed_float3 u;
    float _p1;
    packed_float3 v;
    float _p2;
};

struct Material {
    packed_float3 albedo;
    float roughness;
    packed_float3 emission;
    float metallic;
};

struct SceneUniforms {
    Camera camera;
    uint quad_count;
    uint frame_count;
    uint light_index;
    float _pad;
    Quad quads[MAX_QUADS];
    Material materials[MAX_QUADS];
};

struct Ray {
    float3 origin;
    float3 direction;
};

struct HitRecord {
    float t;
    float3 point;
    float3 normal;
    uint material_index;
    bool hit;
};

// ----- Random Number Generation (PCG) -----

uint pcg_hash(uint input) {
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random_float(thread uint& state) {
    state = pcg_hash(state);
    return float(state) / float(0xFFFFFFFFu);
}

// ----- Sampling -----

float3 cosine_sample_hemisphere(float3 normal, thread uint& rng) {
    float r1 = random_float(rng);
    float r2 = random_float(rng);

    float phi = 2.0f * M_PI_F * r1;
    float cos_theta = sqrt(1.0f - r2);
    float sin_theta = sqrt(r2);

    float3 w = normalize(normal);
    float3 a = (abs(w.x) > 0.9f) ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 v = normalize(cross(w, a));
    float3 u = cross(w, v);

    return normalize(u * cos(phi) * sin_theta + v * sin(phi) * sin_theta + w * cos_theta);
}

// ----- Camera -----

Ray get_camera_ray(Camera cam, float2 uv, thread uint& rng) {
    // Jitter for anti-aliasing.
    float2 jitter = float2(random_float(rng), random_float(rng)) - 0.5f;
    uv += jitter * 0.001f;

    float theta = cam.fov * M_PI_F / 180.0f;
    float h = tan(theta / 2.0f);
    float viewport_height = 2.0f * h;
    float viewport_width = cam.aspect * viewport_height;

    float3 w = normalize(-float3(cam.direction));
    float3 right = normalize(cross(float3(cam.up), w));
    float3 up = cross(w, right);

    float3 horizontal = viewport_width * right;
    float3 vertical = viewport_height * up;
    float3 lower_left = float3(cam.position) - horizontal * 0.5f - vertical * 0.5f - w;

    float3 target = lower_left + uv.x * horizontal + uv.y * vertical;

    Ray ray;
    ray.origin = float3(cam.position);
    ray.direction = normalize(target - ray.origin);
    return ray;
}

// ----- Quad Intersection -----

HitRecord intersect_quad(Ray ray, Quad q, uint mat_idx, float t_min, float t_max) {
    HitRecord rec;
    rec.hit = false;

    float3 origin = float3(q.origin);
    float3 u = float3(q.u);
    float3 v = float3(q.v);

    float3 n = cross(u, v);
    float area_sq = dot(n, n);

    if (area_sq < 1e-8f) return rec;

    float3 normal = n * rsqrt(area_sq);
    float d = dot(normal, origin);

    float denom = dot(normal, ray.direction);
    if (abs(denom) < 1e-8f) return rec;

    float t = (d - dot(normal, ray.origin)) / denom;
    if (t < t_min || t > t_max) return rec;

    float3 p = ray.origin + ray.direction * t;
    float3 planar = p - origin;

    float3 w = n / area_sq;
    float alpha = dot(w, cross(planar, v));
    float beta = dot(w, cross(u, planar));

    if (alpha < 0.0f || alpha > 1.0f || beta < 0.0f || beta > 1.0f) return rec;

    rec.hit = true;
    rec.t = t;
    rec.point = p;
    rec.material_index = mat_idx;
    rec.normal = (denom < 0.0f) ? normal : -normal;

    return rec;
}

// ----- Scene Intersection -----

HitRecord trace_scene(Ray ray, constant SceneUniforms& scene) {
    HitRecord closest;
    closest.hit = false;
    closest.t = 1e30f;

    for (uint i = 0; i < scene.quad_count && i < MAX_QUADS; i++) {
        HitRecord hit = intersect_quad(ray, scene.quads[i], i, 0.001f, closest.t);
        if (hit.hit) {
            closest = hit;
        }
    }

    return closest;
}

// ----- Path Tracing -----

float3 path_trace(Ray ray, constant SceneUniforms& scene, thread uint& rng) {
    float3 color = float3(0.0f);
    float3 throughput = float3(1.0f);

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++) {
        HitRecord hit = trace_scene(ray, scene);

        if (!hit.hit) {
            break;
        }

        Material mat = scene.materials[hit.material_index];

        // Add emission.
        color += throughput * float3(mat.emission);

        // Stop if we hit a light.
        float emit_strength = max(mat.emission.x, max(mat.emission.y, mat.emission.z));
        if (emit_strength > 0.1f) {
            break;
        }

        // Update throughput.
        throughput *= float3(mat.albedo);

        // Russian roulette after a few bounces.
        if (bounce > 2) {
            float p = max(0.05f, max(throughput.x, max(throughput.y, throughput.z)));
            if (random_float(rng) > p) break;
            throughput /= p;
        }

        // New ray direction (cosine-weighted hemisphere).
        ray.origin = hit.point + hit.normal * 0.001f;
        ray.direction = cosine_sample_hemisphere(hit.normal, rng);
    }

    return color;
}

// ----- Main Fragment Shader -----

fragment float4 pathtrace_frag(
    VertexOut in [[stage_in]],
    constant SceneUniforms& scene [[buffer(0)]]
) {
    float2 uv = in.uv;
    uint2 pixel = uint2(in.position.xy);

    // Seed RNG with pixel position and frame count for temporal variation.
    uint rng = pcg_hash(pixel.x + pixel.y * 1920u + scene.frame_count * 1920u * 1080u);

    Ray ray = get_camera_ray(scene.camera, uv, rng);
    float3 color = path_trace(ray, scene, rng);

    return float4(color, 1.0f);
}

// ----- Accumulation Compute Shader -----

kernel void accumulate(
    texture2d<float, access::read> current [[texture(0)]],
    texture2d<float, access::read> history [[texture(1)]],
    texture2d<float, access::write> output [[texture(2)]],
    constant uint& frame_count [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= output.get_width() || gid.y >= output.get_height()) return;

    float4 new_sample = current.read(gid);
    float4 accumulated = history.read(gid);

    // Progressive accumulation: weighted average.
    float weight = 1.0f / float(frame_count + 1);
    float4 result = mix(accumulated, new_sample, weight);

    output.write(result, gid);
}

// ----- Tonemap + Output Shader -----

fragment float4 tonemap_frag(
    VertexOut in [[stage_in]],
    texture2d<float> accumulated [[texture(0)]]
) {
    constexpr sampler s(filter::nearest);
    // Flip Y: Metal textures have (0,0) at top-left, but screen has (0,0) at bottom-left.
    float2 uv = float2(in.uv.x, 1.0f - in.uv.y);
    float3 color = accumulated.sample(s, uv).rgb;

    // Simple Reinhard tonemap.
    color = color / (color + 1.0f);

    // Gamma correction.
    color = pow(color, float3(1.0f / 2.2f));

    return float4(color, 1.0f);
}
)";

// GPU uniform structs - must match shader layout.
struct GpuCamera {
    float position[3];
    float fov;
    float direction[3];
    float aspect;
    float up[3];
    float _pad;
};

struct GpuQuad {
    float origin[3];
    float _p0;
    float u[3];
    float _p1;
    float v[3];
    float _p2;
};

struct GpuMaterial {
    float albedo[3];
    float roughness;
    float emission[3];
    float metallic;
};

struct GpuSceneUniforms {
    GpuCamera camera;
    uint32_t quad_count;
    uint32_t frame_count;
    uint32_t light_index;
    float _pad;
    GpuQuad quads[MAX_QUADS];
    GpuMaterial materials[MAX_QUADS];
};

struct plugin_state {
    Q_plugin_context* context = nullptr;

    id<MTLDevice> device = nil;
    id<MTLRenderPipelineState> pathtrace_pipeline = nil;
    id<MTLRenderPipelineState> tonemap_pipeline = nil;
    id<MTLComputePipelineState> accumulate_pipeline = nil;
    id<MTLTexture> render_target = nil;
    id<MTLTexture> accum_a = nil;
    id<MTLTexture> accum_b = nil;

    Q::scene::cornell_box_scene scene;
    uint32_t frame_count = 0;
    uint32_t last_width = 0;
    uint32_t last_height = 0;
    bool ping = true;
};

void log_msg(plugin_state* state, const char* msg) {
    if (state->context && state->context->log) {
        state->context->log(state->context->host_data, msg);
    }
}

bool create_pipelines(plugin_state* state, MTLPixelFormat output_format) {
    NSError* error = nil;

    NSString* source = [NSString stringWithUTF8String:k_shader_source];
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.fastMathEnabled = YES;

    id<MTLLibrary> library = [state->device newLibraryWithSource:source
                                                         options:options
                                                           error:&error];
    if (!library) {
        NSLog(@"Failed to compile shader: %@", error);
        return false;
    }

    id<MTLFunction> fullscreen_vert = [library newFunctionWithName:@"fullscreen_vert"];
    id<MTLFunction> pathtrace_frag = [library newFunctionWithName:@"pathtrace_frag"];
    id<MTLFunction> tonemap_frag = [library newFunctionWithName:@"tonemap_frag"];
    id<MTLFunction> accumulate_fn = [library newFunctionWithName:@"accumulate"];

    if (!fullscreen_vert || !pathtrace_frag || !tonemap_frag || !accumulate_fn) {
        log_msg(state, "Failed to find shader functions");
        return false;
    }

    // Path trace pipeline (renders to HDR texture).
    MTLRenderPipelineDescriptor* pt_desc = [[MTLRenderPipelineDescriptor alloc] init];
    pt_desc.vertexFunction = fullscreen_vert;
    pt_desc.fragmentFunction = pathtrace_frag;
    pt_desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA32Float;

    state->pathtrace_pipeline = [state->device newRenderPipelineStateWithDescriptor:pt_desc
                                                                              error:&error];
    if (!state->pathtrace_pipeline) {
        NSLog(@"Failed to create pathtrace pipeline: %@", error);
        return false;
    }

    // Tonemap pipeline (renders to screen).
    MTLRenderPipelineDescriptor* tm_desc = [[MTLRenderPipelineDescriptor alloc] init];
    tm_desc.vertexFunction = fullscreen_vert;
    tm_desc.fragmentFunction = tonemap_frag;
    tm_desc.colorAttachments[0].pixelFormat = output_format;

    state->tonemap_pipeline = [state->device newRenderPipelineStateWithDescriptor:tm_desc
                                                                            error:&error];
    if (!state->tonemap_pipeline) {
        NSLog(@"Failed to create tonemap pipeline: %@", error);
        return false;
    }

    // Accumulation compute pipeline.
    state->accumulate_pipeline = [state->device newComputePipelineStateWithFunction:accumulate_fn
                                                                              error:&error];
    if (!state->accumulate_pipeline) {
        NSLog(@"Failed to create accumulate pipeline: %@", error);
        return false;
    }

    return true;
}

void create_textures(plugin_state* state, uint32_t width, uint32_t height) {
    MTLTextureDescriptor* hdr_desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                     width:width
                                    height:height
                                 mipmapped:NO];
    hdr_desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    hdr_desc.storageMode = MTLStorageModePrivate;

    state->render_target = [state->device newTextureWithDescriptor:hdr_desc];

    // Accumulation buffers need read/write.
    hdr_desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    state->accum_a = [state->device newTextureWithDescriptor:hdr_desc];
    state->accum_b = [state->device newTextureWithDescriptor:hdr_desc];

    state->last_width = width;
    state->last_height = height;
    state->frame_count = 0;
    state->ping = true;
}

}  // namespace

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
            ctx->log(ctx->host_data, "Metal backend requires Metal GPU context");
        }
        return nullptr;
    }

    auto* state = new plugin_state{};
    state->context = ctx;
    state->device = (__bridge id<MTLDevice>)ctx->gpu->device;

    // Get pixel format from layer.
    CAMetalLayer* layer = (__bridge CAMetalLayer*)ctx->gpu->layer;
    MTLPixelFormat format = layer.pixelFormat;

    if (!create_pipelines(state, format)) {
        delete state;
        return nullptr;
    }

    // Set up Cornell Box scene.
    float aspect = static_cast<float>(ctx->viewport_width) /
                   static_cast<float>(ctx->viewport_height);
    state->scene = Q::scene::make_cornell_box(aspect);

    // Create render textures.
    create_textures(state, ctx->viewport_width, ctx->viewport_height);

    log_msg(state, "Cornell Box path tracer initialized");

    return reinterpret_cast<Q_plugin_handle*>(state);
}

Q_EXPORT void Q_plugin_destroy(Q_plugin_handle* handle) {
    if (!handle) return;
    auto* state = reinterpret_cast<plugin_state*>(handle);

    log_msg(state, "Path tracer destroyed");

    state->pathtrace_pipeline = nil;
    state->tonemap_pipeline = nil;
    state->accumulate_pipeline = nil;
    state->render_target = nil;
    state->accum_a = nil;
    state->accum_b = nil;

    delete state;
}

Q_EXPORT void Q_plugin_update(Q_plugin_handle* handle, float delta_time) {
    (void)handle;
    (void)delta_time;
}

Q_EXPORT void Q_plugin_render(Q_plugin_handle* handle, Q_render_frame* frame) {
    if (!handle || !frame) return;

    auto* state = reinterpret_cast<plugin_state*>(handle);

    id<CAMetalDrawable> drawable = (__bridge id<CAMetalDrawable>)frame->drawable;
    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)frame->command_buffer;

    if (!drawable || !cmd_buf) return;

    uint32_t width = frame->width;
    uint32_t height = frame->height;

    // Recreate textures if size changed.
    if (width != state->last_width || height != state->last_height) {
        create_textures(state, width, height);
    }

    // Update aspect ratio.
    state->scene.cam.aspect = static_cast<float>(width) / static_cast<float>(height);

    // Build uniforms.
    GpuSceneUniforms uniforms{};

    const auto& cam = state->scene.cam;
    uniforms.camera.position[0] = cam.position.x;
    uniforms.camera.position[1] = cam.position.y;
    uniforms.camera.position[2] = cam.position.z;
    uniforms.camera.direction[0] = cam.direction.x;
    uniforms.camera.direction[1] = cam.direction.y;
    uniforms.camera.direction[2] = cam.direction.z;
    uniforms.camera.up[0] = cam.up.x;
    uniforms.camera.up[1] = cam.up.y;
    uniforms.camera.up[2] = cam.up.z;
    uniforms.camera.fov = cam.fov;
    uniforms.camera.aspect = cam.aspect;

    uniforms.quad_count = static_cast<uint32_t>(std::min(state->scene.quads.size(), size_t(MAX_QUADS)));
    uniforms.frame_count = state->frame_count;
    uniforms.light_index = static_cast<uint32_t>(state->scene.light_index);

    for (size_t i = 0; i < uniforms.quad_count; i++) {
        const auto& q = state->scene.quads[i];
        uniforms.quads[i].origin[0] = q.geometry.origin.x;
        uniforms.quads[i].origin[1] = q.geometry.origin.y;
        uniforms.quads[i].origin[2] = q.geometry.origin.z;
        uniforms.quads[i].u[0] = q.geometry.u.x;
        uniforms.quads[i].u[1] = q.geometry.u.y;
        uniforms.quads[i].u[2] = q.geometry.u.z;
        uniforms.quads[i].v[0] = q.geometry.v.x;
        uniforms.quads[i].v[1] = q.geometry.v.y;
        uniforms.quads[i].v[2] = q.geometry.v.z;

        uniforms.materials[i].albedo[0] = q.mat.albedo.x;
        uniforms.materials[i].albedo[1] = q.mat.albedo.y;
        uniforms.materials[i].albedo[2] = q.mat.albedo.z;
        uniforms.materials[i].roughness = q.mat.roughness;
        uniforms.materials[i].emission[0] = q.mat.emission.x;
        uniforms.materials[i].emission[1] = q.mat.emission.y;
        uniforms.materials[i].emission[2] = q.mat.emission.z;
        uniforms.materials[i].metallic = q.mat.metallic;
    }

    // Select accumulation buffers.
    id<MTLTexture> read_accum = state->ping ? state->accum_a : state->accum_b;
    id<MTLTexture> write_accum = state->ping ? state->accum_b : state->accum_a;

    // 1. Path trace pass - render new sample.
    {
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = state->render_target;
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:pass];
        [enc setRenderPipelineState:state->pathtrace_pipeline];
        [enc setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [enc endEncoding];
    }

    // 2. Accumulate pass - blend new sample with history.
    {
        id<MTLComputeCommandEncoder> enc = [cmd_buf computeCommandEncoder];
        [enc setComputePipelineState:state->accumulate_pipeline];
        [enc setTexture:state->render_target atIndex:0];
        [enc setTexture:read_accum atIndex:1];
        [enc setTexture:write_accum atIndex:2];
        [enc setBytes:&state->frame_count length:sizeof(uint32_t) atIndex:0];

        MTLSize grid = MTLSizeMake(width, height, 1);
        MTLSize group = MTLSizeMake(16, 16, 1);
        [enc dispatchThreads:grid threadsPerThreadgroup:group];
        [enc endEncoding];
    }

    // 3. Tonemap pass - output to screen.
    {
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = drawable.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:pass];
        [enc setRenderPipelineState:state->tonemap_pipeline];
        [enc setFragmentTexture:write_accum atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [enc endEncoding];
    }

    state->ping = !state->ping;
    state->frame_count++;
}

}  // extern "C"
