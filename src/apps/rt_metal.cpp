#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <quasi/geometry/geometry.hpp>
#include <quasi/io/ppm_writer.hpp>
#include <quasi/io/scene_parser.hpp>
#include <quasi/radiometry/camera.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/scene/scene.hpp>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;
using namespace Q::scene;

// Maximum limits matching Metal shader
constexpr int MAX_SPHERES = 32;
constexpr int MAX_TRIANGLES = 128;
constexpr int MAX_LIGHTS = 16;

// Metal-compatible structures (matching shader)
struct MetalRay {
  simd_float3 origin;
  simd_float3 direction;
};

struct MetalSphere {
  simd_float3 center;
  float radius;
  simd_float3 color;
  float reflectivity;
};

struct MetalTriangle {
  simd_float3 v0, v1, v2;
  simd_float3 normal;
  simd_float3 color;
  float reflectivity;
};

struct MetalLight {
  simd_float3 position;
  simd_float3 color;
  float intensity;
  int type; // 0 = point, 1 = area
  float width, height;
  int samples;
  float padding[2]; // Ensure 16-byte alignment
};

struct MetalCamera {
  simd_float3 position;
  simd_float3 look_at;
  simd_float3 up;
  float fov;
  float aspect_ratio;
  float padding[3]; // Ensure 16-byte alignment
};

struct MetalSceneData {
  MetalCamera camera;
  int num_spheres;
  int num_triangles;
  int num_lights;
  int padding; // Ensure alignment
  MetalSphere spheres[MAX_SPHERES];
  MetalTriangle triangles[MAX_TRIANGLES];
  MetalLight lights[MAX_LIGHTS];
  simd_float3 background_color;
  int samples_per_pixel;
};

struct MetalRenderParams {
  uint32_t width;
  uint32_t height;
  uint32_t samples_per_pixel;
  uint32_t padding; // Ensure 16-byte alignment
};

// Utility functions to convert between Q types and Metal types
simd_float3 to_simd(const Vec3 &v) {
  return simd_make_float3(v.x, v.y, v.z);
}

simd_float3 to_simd(const Color &c) {
  return simd_make_float3(c.r, c.g, c.b);
}

Color from_simd(const simd_float3 &v) {
  return Color(v.x, v.y, v.z);
}

class MetalRayTracer {
private:
  id<MTLDevice> device_;
  id<MTLCommandQueue> command_queue_;
  id<MTLLibrary> library_;
  id<MTLComputePipelineState> pipeline_state_;
  id<MTLBuffer> scene_buffer_;
  id<MTLBuffer> params_buffer_;

public:
  MetalRayTracer() { initialize_metal(); }

  ~MetalRayTracer() { cleanup(); }

  void initialize_metal() {
    // Get the default Metal device
    device_ = MTLCreateSystemDefaultDevice();
    if (!device_) {
      throw std::runtime_error("Metal is not supported on this system");
    }

    std::cout << "Metal Device: " << [device_.name UTF8String] << std::endl;

    // Create command queue
    command_queue_ = [device_ newCommandQueue];
    if (!command_queue_) {
      throw std::runtime_error("Failed to create Metal command queue");
    }

    // Load the Metal shader library
    NSError *error = nil;
    NSString *source = nil;

    // Try multiple paths to find the shader
    NSArray *possible_paths = @[
      @"debug.metal", @"raytracer.metal", @"src/apps/raytracer.metal",
      @"build/quasi-build/src/apps/raytracer.metal", @"quasi-build/src/apps/debug.metal"
    ];

    NSString *current_dir = [[NSFileManager defaultManager] currentDirectoryPath];

    for (NSString *relative_path in possible_paths) {
      NSString *full_path = [current_dir stringByAppendingPathComponent:relative_path];
      source =
          [NSString stringWithContentsOfFile:full_path encoding:NSUTF8StringEncoding error:&error];
      if (source) {
        std::cout << "Found shader at: " << [full_path UTF8String] << std::endl;
        break;
      }
    }

    if (!source) {
      std::string error_msg = "Failed to load Metal shader source. Searched in:\n";
      for (NSString *relative_path in possible_paths) {
        NSString *full_path = [current_dir stringByAppendingPathComponent:relative_path];
        error_msg += "  " + std::string([full_path UTF8String]) + "\n";
      }
      throw std::runtime_error(error_msg);
    }

    library_ = [device_ newLibraryWithSource:source options:nil error:&error];
    if (!library_) {
      NSString *error_desc = [error localizedDescription];
      std::string error_str = [error_desc UTF8String];
      throw std::runtime_error("Failed to create Metal library: " + error_str);
    }

    // Get the compute function
    id<MTLFunction> compute_function = [library_ newFunctionWithName:@"raytracer_kernel"];
    if (!compute_function) {
      throw std::runtime_error("Failed to find compute function 'raytracer_kernel'");
    }

    // Create compute pipeline state
    pipeline_state_ = [device_ newComputePipelineStateWithFunction:compute_function error:&error];
    if (!pipeline_state_) {
      NSString *error_desc = [error localizedDescription];
      std::string error_str = [error_desc UTF8String];
      throw std::runtime_error("Failed to create compute pipeline state: " + error_str);
    }

    std::cout << "Metal ray tracer initialized successfully" << std::endl;
  }

  void cleanup() {
    // Metal objects are automatically released with ARC
    // No explicit cleanup needed in modern Objective-C++
  }

  MetalSceneData convert_scene_data(const SceneData &scene_data, const Scene &scene) {
    MetalSceneData metal_scene = {};

    // Convert camera
    metal_scene.camera.position = to_simd(scene_data.camera.position);
    metal_scene.camera.look_at = to_simd(scene_data.camera.look_at);
    metal_scene.camera.up = to_simd(scene_data.camera.up);
    metal_scene.camera.fov = scene_data.camera.fov;
    metal_scene.camera.aspect_ratio =
        static_cast<float>(scene_data.render.width) / static_cast<float>(scene_data.render.height);

    // Convert spheres
    metal_scene.num_spheres = std::min(static_cast<int>(scene_data.spheres.size()), MAX_SPHERES);
    for (int i = 0; i < metal_scene.num_spheres; i++) {
      const auto &sphere = scene_data.spheres[i];
      metal_scene.spheres[i].center = to_simd(sphere.center);
      metal_scene.spheres[i].radius = sphere.radius;
      metal_scene.spheres[i].color = to_simd(sphere.color);
      metal_scene.spheres[i].reflectivity = sphere.reflectance;
    }

    // Convert triangles from scene triangles and boxes
    metal_scene.num_triangles = 0;

    // Add scene triangles
    for (const auto &triangle : scene_data.triangles) {
      if (metal_scene.num_triangles >= MAX_TRIANGLES)
        break;

      metal_scene.triangles[metal_scene.num_triangles].v0 = to_simd(triangle.vertex1);
      metal_scene.triangles[metal_scene.num_triangles].v1 = to_simd(triangle.vertex2);
      metal_scene.triangles[metal_scene.num_triangles].v2 = to_simd(triangle.vertex3);

      // Calculate normal from vertices
      Vec3 edge1 = triangle.vertex2 - triangle.vertex1;
      Vec3 edge2 = triangle.vertex3 - triangle.vertex1;
      Vec3 normal = edge1.cross_product(edge2).get_normalized();
      metal_scene.triangles[metal_scene.num_triangles].normal = to_simd(normal);

      metal_scene.triangles[metal_scene.num_triangles].color = to_simd(triangle.color);
      metal_scene.triangles[metal_scene.num_triangles].reflectivity = triangle.reflectance;
      metal_scene.num_triangles++;
    }

    // Convert boxes to triangles (each box becomes 12 triangles)
    for (const auto &box : scene_data.boxes) {
      Q::geometry::Box geometry_box(box.min_corner, box.max_corner);
      auto box_triangles = geometry_box.get_triangles();

      for (const auto &triangle : box_triangles) {
        if (metal_scene.num_triangles >= MAX_TRIANGLES)
          break;

        metal_scene.triangles[metal_scene.num_triangles].v0 = to_simd(triangle.v0);
        metal_scene.triangles[metal_scene.num_triangles].v1 = to_simd(triangle.v1);
        metal_scene.triangles[metal_scene.num_triangles].v2 = to_simd(triangle.v2);

        // Calculate normal from vertices
        Vec3 edge1 = triangle.v1 - triangle.v0;
        Vec3 edge2 = triangle.v2 - triangle.v0;
        Vec3 normal = edge1.cross_product(edge2).get_normalized();
        metal_scene.triangles[metal_scene.num_triangles].normal = to_simd(normal);

        metal_scene.triangles[metal_scene.num_triangles].color = to_simd(box.color);
        metal_scene.triangles[metal_scene.num_triangles].reflectivity = box.reflectance;
        metal_scene.num_triangles++;
      }
    }

    // Convert lights
    metal_scene.num_lights = std::min(static_cast<int>(scene_data.lights.size()), MAX_LIGHTS);
    for (int i = 0; i < metal_scene.num_lights; i++) {
      const auto &light = scene_data.lights[i];
      metal_scene.lights[i].position = to_simd(light.position);
      metal_scene.lights[i].color = to_simd(light.color);
      metal_scene.lights[i].intensity = light.intensity;
      metal_scene.lights[i].type =
          (light.type == "area_light" || light.type == "rectangular_area_light") ? 1 : 0;
      metal_scene.lights[i].width = light.width;
      metal_scene.lights[i].height = light.height;
      metal_scene.lights[i].samples = light.samples;
    }

    // Set background color (use color1 as the primary background)
    metal_scene.background_color = to_simd(scene_data.background.color1);
    metal_scene.samples_per_pixel = scene_data.render.multisampling.samples_per_pixel;

    return metal_scene;
  }

  std::vector<Color> render(const SceneData &scene_data, const Scene &scene) {
    auto start_time = std::chrono::high_resolution_clock::now();

    const uint32_t width = scene_data.render.width;
    const uint32_t height = scene_data.render.height;
    const uint32_t samples_per_pixel = scene_data.render.multisampling.samples_per_pixel;

    std::cout << "Rendering " << width << "x" << height << " image using Metal GPU..." << std::endl;

    // Create output texture with error checking
    MTLTextureDescriptor *texture_desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                           width:width
                                                          height:height
                                                       mipmapped:NO];
    texture_desc.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
    id<MTLTexture> output_texture = [device_ newTextureWithDescriptor:texture_desc];
    if (!output_texture) {
      throw std::runtime_error("Failed to create Metal output texture");
    }
    std::cout << "Output texture created: " << width << "x" << height << std::endl;

    // Create command buffer with error checking
    id<MTLCommandBuffer> command_buffer = [command_queue_ commandBuffer];
    if (!command_buffer) {
      throw std::runtime_error("Failed to create Metal command buffer");
    }

    id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
    if (!compute_encoder) {
      throw std::runtime_error("Failed to create Metal compute encoder");
    }
    std::cout << "Command buffer and encoder created" << std::endl;

    // Convert scene data to Metal format
    MetalSceneData metal_scene = convert_scene_data(scene_data, scene);

    // Debug: Print scene data being sent to Metal
    std::cout << "Scene data sent to Metal:" << std::endl;
    std::cout << "  Camera position: (" << metal_scene.camera.position.x << ", "
              << metal_scene.camera.position.y << ", " << metal_scene.camera.position.z << ")"
              << std::endl;
    std::cout << "  Camera look_at: (" << metal_scene.camera.look_at.x << ", "
              << metal_scene.camera.look_at.y << ", " << metal_scene.camera.look_at.z << ")"
              << std::endl;
    std::cout << "  FOV: " << metal_scene.camera.fov
              << ", Aspect: " << metal_scene.camera.aspect_ratio << std::endl;
    std::cout << "  Background: (" << metal_scene.background_color.x << ", "
              << metal_scene.background_color.y << ", " << metal_scene.background_color.z << ")"
              << std::endl;
    std::cout << "  Spheres: " << metal_scene.num_spheres << std::endl;
    for (int i = 0; i < metal_scene.num_spheres; i++) {
      const auto &sphere = metal_scene.spheres[i];
      std::cout << "    Sphere " << i << ": center(" << sphere.center.x << ", " << sphere.center.y
                << ", " << sphere.center.z << "), radius=" << sphere.radius << ", color=("
                << sphere.color.x << ", " << sphere.color.y << ", " << sphere.color.z << ")"
                << std::endl;
    }
    std::cout << "  Triangles: " << metal_scene.num_triangles << std::endl;
    std::cout << "  Lights: " << metal_scene.num_lights << std::endl;
    for (int i = 0; i < metal_scene.num_lights; i++) {
      const auto &light = metal_scene.lights[i];
      std::cout << "    Light " << i << ": pos(" << light.position.x << ", " << light.position.y
                << ", " << light.position.z << "), intensity=" << light.intensity
                << ", type=" << light.type << std::endl;
    }

    // Create scene data buffer
    scene_buffer_ = [device_ newBufferWithBytes:&metal_scene
                                         length:sizeof(MetalSceneData)
                                        options:MTLResourceStorageModeShared];
    if (!scene_buffer_) {
      throw std::runtime_error("Failed to create Metal scene buffer");
    }

    // Create render parameters buffer
    MetalRenderParams render_params = {width, height, samples_per_pixel, 0};
    params_buffer_ = [device_ newBufferWithBytes:&render_params
                                          length:sizeof(MetalRenderParams)
                                         options:MTLResourceStorageModeShared];
    if (!params_buffer_) {
      throw std::runtime_error("Failed to create Metal params buffer");
    }

    // Set up compute pipeline
    [compute_encoder setComputePipelineState:pipeline_state_];
    [compute_encoder setTexture:output_texture atIndex:0];
    [compute_encoder setBuffer:scene_buffer_ offset:0 atIndex:0];
    [compute_encoder setBuffer:params_buffer_ offset:0 atIndex:1];

    // Calculate thread group sizes with validation
    NSUInteger thread_group_size = pipeline_state_.maxTotalThreadsPerThreadgroup;
    NSUInteger thread_group_width = 16; // Good for 2D work
    NSUInteger thread_group_height = thread_group_size / thread_group_width;

    std::cout << "Thread group size: " << thread_group_size << std::endl;
    std::cout << "Thread group dimensions: " << thread_group_width << "x" << thread_group_height
              << std::endl;

    MTLSize threads_per_threadgroup = MTLSizeMake(thread_group_width, thread_group_height, 1);
    MTLSize threads_per_grid = MTLSizeMake(width, height, 1);

    std::cout << "Dispatching " << width << "x" << height << " threads" << std::endl;

    // Dispatch compute work
    [compute_encoder dispatchThreads:threads_per_grid
               threadsPerThreadgroup:threads_per_threadgroup];

    [compute_encoder endEncoding];

    // Execute the command buffer
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    // Check for errors
    if (command_buffer.status == MTLCommandBufferStatusError) {
      NSError *error = command_buffer.error;
      if (error) {
        std::string error_str = [error.localizedDescription UTF8String];
        throw std::runtime_error("Metal compute command failed: " + error_str);
      }
      throw std::runtime_error("Metal compute command failed with unknown error");
    }

    // Read back the results
    std::vector<Color> pixels(width * height);
    std::vector<simd_float4> texture_data(width * height);

    [output_texture getBytes:texture_data.data()
                 bytesPerRow:width * sizeof(simd_float4)
               bytesPerImage:width * height * sizeof(simd_float4)
                  fromRegion:MTLRegionMake2D(0, 0, width, height)
                 mipmapLevel:0
                       slice:0];

    // Debug: Check what we actually got from Metal
    std::cout << "First 10 pixels from Metal GPU:" << std::endl;
    for (int i = 0; i < std::min(10, (int) texture_data.size()); i++) {
      const simd_float4 &texel = texture_data[i];
      std::cout << "  GPU Pixel " << i << ": (" << texel.x << ", " << texel.y << ", " << texel.z
                << ", " << texel.w << ")" << std::endl;
    }

    // Convert texture data to Color objects
    for (size_t i = 0; i < pixels.size(); i++) {
      const simd_float4 &texel = texture_data[i];
      pixels[i] = Color(texel.x, texel.y, texel.z);
    }

    // Debug: Check what Color objects we created
    std::cout << "First 10 Color objects:" << std::endl;
    for (int i = 0; i < std::min(10, (int) pixels.size()); i++) {
      const Color &color = pixels[i];
      std::cout << "  Color " << i << ": (" << color.r << ", " << color.g << ", " << color.b << ")"
                << std::endl;
    }

    // Calculate and display timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    long long ms = duration.count();
    double seconds = ms / 1000.0;
    long long total_rays = static_cast<long long>(width) * height * samples_per_pixel;
    long long rays_per_second = static_cast<long long>(total_rays / seconds);

    if (ms < 1000) {
      std::cout << total_rays << " rays traced in " << ms << " ms at " << rays_per_second
                << " rays/s using Metal GPU" << std::endl;
    } else if (ms < 60000) {
      std::cout << total_rays << " rays traced in " << std::fixed << std::setprecision(1) << seconds
                << " s at " << rays_per_second << " rays/s using Metal GPU" << std::endl;
    } else {
      int minutes = ms / 60000;
      double remaining_seconds = (ms % 60000) / 1000.0;
      std::cout << total_rays << " rays traced in " << minutes << " min " << std::fixed
                << std::setprecision(1) << remaining_seconds << " s at " << rays_per_second
                << " rays/s using Metal GPU" << std::endl;
    }

    return pixels;
  }
};

int main(int argc, char *argv[]) {
  try {
    // Parse command line arguments
    std::string scene_file = "data/scenes/cornell_box.json";
    std::string output_file = "/Users/tt/Desktop/metal_render.ppm";

    if (argc >= 2) {
      scene_file = argv[1];
    }
    if (argc >= 3) {
      output_file = argv[2];
    }

    std::cout << "Metal Ray Tracer - rendering scene: " << scene_file << std::endl;

    // Parse scene file
    SceneData scene_data = SceneParser::parse_scene_file(scene_file);
    Scene scene(scene_data);

    // Create Metal ray tracer
    MetalRayTracer metal_ray_tracer;

    // Render using Metal
    std::vector<Color> pixels = metal_ray_tracer.render(scene_data, scene);

    // Write the image to file with tone mapping
    PPMWriter::write_ppm(output_file, pixels, scene_data.render.width, scene_data.render.height,
                         ToneMapType::REINHARD, 0.0f, 2.2f);

    std::cout << "Render complete! Output saved to: " << output_file << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
