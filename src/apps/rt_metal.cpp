#include <Metal/Metal.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <quasi/geometry/geometry.hpp>
#include <quasi/io/scene_parser.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/scene/scene.hpp>
#include <simd/simd.h>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;
using namespace Q::scene;

// Metal structures matching the shader definitions
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
  MetalSphere spheres[32];
  MetalTriangle triangles[128];
  MetalLight lights[16];
  simd_float3 background_color;
  int samples_per_pixel;
};

struct MetalRenderParams {
  uint32_t width;
  uint32_t height;
  uint32_t samples_per_pixel;
  uint32_t padding; // Ensure 16-byte alignment
};

// Convert scene data to Metal format
MetalSceneData convertToMetalScene(const Q::io::SceneData &scene_data, uint32_t width,
                                   uint32_t height) {
  MetalSceneData metalScene = {};

  // Camera
  metalScene.camera.position = simd_make_float3(
      scene_data.camera.position.x, scene_data.camera.position.y, scene_data.camera.position.z);
  metalScene.camera.look_at = simd_make_float3(
      scene_data.camera.look_at.x, scene_data.camera.look_at.y, scene_data.camera.look_at.z);
  metalScene.camera.up =
      simd_make_float3(scene_data.camera.up.x, scene_data.camera.up.y, scene_data.camera.up.z);
  metalScene.camera.fov = scene_data.camera.fov;
  metalScene.camera.aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

  // Background color
  metalScene.background_color =
      simd_make_float3(scene_data.background.color1.r, scene_data.background.color1.g,
                       scene_data.background.color1.b);

  // Objects (spheres)
  metalScene.num_spheres = std::min(static_cast<int>(scene_data.spheres.size()), 32);
  for (int i = 0; i < metalScene.num_spheres; i++) {
    const auto &sphere = scene_data.spheres[i];
    metalScene.spheres[i].center =
        simd_make_float3(sphere.center.x, sphere.center.y, sphere.center.z);
    metalScene.spheres[i].radius = sphere.radius;
    metalScene.spheres[i].color = simd_make_float3(sphere.color.r, sphere.color.g, sphere.color.b);
    metalScene.spheres[i].reflectivity = sphere.reflectance;
  }

  // Triangles from boxes
  metalScene.num_triangles = 0;
  for (const auto &box : scene_data.boxes) {
    if (metalScene.num_triangles >= 120)
      break; // Leave room for safety

    Vec3 min = box.min_corner;
    Vec3 max = box.max_corner;
    Color color = box.color;
    float reflectance = box.reflectance;

    // Generate 12 triangles for the box (2 triangles per face, 6 faces)
    std::vector<Vec3> vertices = {// Front face (z = max.z)
                                  Vec3(min.x, min.y, max.z), Vec3(max.x, min.y, max.z),
                                  Vec3(max.x, max.y, max.z), Vec3(min.x, max.y, max.z),
                                  // Back face (z = min.z)
                                  Vec3(max.x, min.y, min.z), Vec3(min.x, min.y, min.z),
                                  Vec3(min.x, max.y, min.z), Vec3(max.x, max.y, min.z)};

    std::vector<std::tuple<int, int, int, Vec3>> faces = {// Front face
                                                          {0, 1, 2, Vec3(0, 0, 1)},
                                                          {0, 2, 3, Vec3(0, 0, 1)},
                                                          // Back face
                                                          {4, 5, 6, Vec3(0, 0, -1)},
                                                          {4, 6, 7, Vec3(0, 0, -1)},
                                                          // Left face
                                                          {5, 0, 3, Vec3(-1, 0, 0)},
                                                          {5, 3, 6, Vec3(-1, 0, 0)},
                                                          // Right face
                                                          {1, 4, 7, Vec3(1, 0, 0)},
                                                          {1, 7, 2, Vec3(1, 0, 0)},
                                                          // Bottom face
                                                          {5, 4, 1, Vec3(0, -1, 0)},
                                                          {5, 1, 0, Vec3(0, -1, 0)},
                                                          // Top face
                                                          {3, 2, 7, Vec3(0, 1, 0)},
                                                          {3, 7, 6, Vec3(0, 1, 0)}};

    for (const auto &face : faces) {
      if (metalScene.num_triangles >= 128)
        break;

      auto [i0, i1, i2, normal] = face;
      MetalTriangle &tri = metalScene.triangles[metalScene.num_triangles];

      tri.v0 = simd_make_float3(vertices[i0].x, vertices[i0].y, vertices[i0].z);
      tri.v1 = simd_make_float3(vertices[i1].x, vertices[i1].y, vertices[i1].z);
      tri.v2 = simd_make_float3(vertices[i2].x, vertices[i2].y, vertices[i2].z);
      tri.normal = simd_make_float3(normal.x, normal.y, normal.z);
      tri.color = simd_make_float3(color.r, color.g, color.b);
      tri.reflectivity = reflectance;

      metalScene.num_triangles++;
    }
  }

  // Lights
  metalScene.num_lights = std::min(static_cast<int>(scene_data.lights.size()), 16);
  for (int i = 0; i < metalScene.num_lights; i++) {
    const auto &light = scene_data.lights[i];
    metalScene.lights[i].position =
        simd_make_float3(light.position.x, light.position.y, light.position.z);
    metalScene.lights[i].color = simd_make_float3(light.color.r, light.color.g, light.color.b);
    metalScene.lights[i].intensity = light.intensity;
    metalScene.lights[i].type = (light.type == "point_light") ? 0 : 1;
    metalScene.lights[i].width = light.width;
    metalScene.lights[i].height = light.height;
    metalScene.lights[i].samples = light.samples;
  }

  return metalScene;
}

void writeImageToPPM(const std::vector<Color> &pixels, uint32_t width, uint32_t height,
                     const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
    return;
  }

  file << "P3\n";
  file << width << " " << height << "\n";
  file << "255\n";

  for (const auto &pixel : pixels) {
    int r = static_cast<int>(std::clamp(pixel.r * 255.0f, 0.0f, 255.0f));
    int g = static_cast<int>(std::clamp(pixel.g * 255.0f, 0.0f, 255.0f));
    int b = static_cast<int>(std::clamp(pixel.b * 255.0f, 0.0f, 255.0f));
    file << r << " " << g << " " << b << "\n";
  }

  file.close();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <scene_file> [output_file]" << std::endl;
    return 1;
  }

  std::string scene_file = argv[1];
  std::string output_file = (argc > 2) ? argv[2] : "output.ppm";

  std::cout << "Metal Ray Tracer - rendering scene: " << scene_file << std::endl;

  // Parse scene
  Q::io::SceneData scene_data;
  try {
    scene_data = SceneParser::parse_scene_file(scene_file);
  } catch (const std::exception &e) {
    std::cerr << "Error parsing scene: " << e.what() << std::endl;
    return 1;
  }

  // Display parsed scene info
  std::cout << "Processing " << scene_data.lights.size() << " lights from scene data" << std::endl;
  for (size_t i = 0; i < scene_data.lights.size(); i++) {
    const auto &light = scene_data.lights[i];
    if (light.type == "point_light") {
      std::cout << "Creating point light at position (" << light.position.x << ", "
                << light.position.y << ", " << light.position.z << ") with type: point_light"
                << std::endl;
    } else {
      std::cout << "Creating rectangular area light at position (" << light.position.x << ", "
                << light.position.y << ", " << light.position.z << ") with size " << light.width
                << "x" << light.height << " and " << light.samples
                << " samples using stratified sampling" << std::endl;
    }
  }

  // Get render dimensions
  uint32_t width = scene_data.render.width;
  uint32_t height = scene_data.render.height;

  // Initialize Metal
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    std::cerr << "Error: Metal is not supported on this device" << std::endl;
    return 1;
  }
  std::cout << "Metal Device: " << [device.name UTF8String] << std::endl;

  // Find shader file
  std::string shader_path = "/Users/tt/src/Quasi/build/raytracer.metal";
  if (!std::filesystem::exists(shader_path)) {
    std::cerr << "Error: Shader file not found at " << shader_path << std::endl;
    return 1;
  }
  std::cout << "Found shader at: " << shader_path << std::endl;

  // Load and compile shader
  NSError *error = nil;
  NSString *shaderSource =
      [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:shader_path.c_str()]
                                encoding:NSUTF8StringEncoding
                                   error:&error];
  if (error) {
    std::cerr << "Error reading shader file: " << [error.localizedDescription UTF8String]
              << std::endl;
    return 1;
  }

  id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
  if (error) {
    std::cerr << "Error compiling shader: " << [error.localizedDescription UTF8String] << std::endl;
    return 1;
  }

  id<MTLFunction> kernelFunction = [library newFunctionWithName:@"raytracer_kernel"];
  if (!kernelFunction) {
    std::cerr << "Error: Could not find raytracer_kernel function in shader" << std::endl;
    return 1;
  }

  id<MTLComputePipelineState> pipelineState =
      [device newComputePipelineStateWithFunction:kernelFunction error:&error];
  if (error) {
    std::cerr << "Error creating pipeline state: " << [error.localizedDescription UTF8String]
              << std::endl;
    return 1;
  }

  std::cout << "Metal ray tracer initialized successfully" << std::endl;

  // Create command queue
  id<MTLCommandQueue> commandQueue = [device newCommandQueue];

  std::cout << "Rendering " << width << "x" << height << " image using Metal GPU..." << std::endl;

  // Create output texture
  MTLTextureDescriptor *textureDescriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                         width:width
                                                        height:height
                                                     mipmapped:NO];
  textureDescriptor.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
  id<MTLTexture> outputTexture = [device newTextureWithDescriptor:textureDescriptor];
  std::cout << "Output texture created: " << width << "x" << height << std::endl;

  // Convert scene to Metal format
  MetalSceneData metalScene = convertToMetalScene(scene_data, width, height);
  MetalRenderParams renderParams = {width, height, 1, 0};

  // Create buffers
  id<MTLBuffer> sceneBuffer = [device newBufferWithBytes:&metalScene
                                                  length:sizeof(MetalSceneData)
                                                 options:MTLResourceStorageModeShared];

  id<MTLBuffer> paramsBuffer = [device newBufferWithBytes:&renderParams
                                                   length:sizeof(MetalRenderParams)
                                                  options:MTLResourceStorageModeShared];

  // Create command buffer and encoder
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
  std::cout << "Command buffer and encoder created" << std::endl;

  // Debug scene data
  std::cout << "Scene data sent to Metal:" << std::endl;
  std::cout << "  Camera position: (" << metalScene.camera.position.x << ", "
            << metalScene.camera.position.y << ", " << metalScene.camera.position.z << ")"
            << std::endl;
  std::cout << "  Camera look_at: (" << metalScene.camera.look_at.x << ", "
            << metalScene.camera.look_at.y << ", " << metalScene.camera.look_at.z << ")"
            << std::endl;
  std::cout << "  FOV: " << metalScene.camera.fov << ", Aspect: " << metalScene.camera.aspect_ratio
            << std::endl;
  std::cout << "  Background: (" << metalScene.background_color.x << ", "
            << metalScene.background_color.y << ", " << metalScene.background_color.z << ")"
            << std::endl;
  std::cout << "  Spheres: " << metalScene.num_spheres << std::endl;
  for (int i = 0; i < metalScene.num_spheres; i++) {
    std::cout << "    Sphere " << i << ": center(" << metalScene.spheres[i].center.x << ", "
              << metalScene.spheres[i].center.y << ", " << metalScene.spheres[i].center.z
              << "), radius=" << metalScene.spheres[i].radius << ", color=("
              << metalScene.spheres[i].color.x << ", " << metalScene.spheres[i].color.y << ", "
              << metalScene.spheres[i].color.z << ")" << std::endl;
  }
  std::cout << "  Triangles: " << metalScene.num_triangles << std::endl;
  std::cout << "  Lights: " << metalScene.num_lights << std::endl;
  for (int i = 0; i < metalScene.num_lights; i++) {
    std::cout << "    Light " << i << ": pos(" << metalScene.lights[i].position.x << ", "
              << metalScene.lights[i].position.y << ", " << metalScene.lights[i].position.z
              << "), intensity=" << metalScene.lights[i].intensity
              << ", type=" << metalScene.lights[i].type << std::endl;
  }

  // Set compute pipeline and buffers
  [encoder setComputePipelineState:pipelineState];
  [encoder setTexture:outputTexture atIndex:0];
  [encoder setBuffer:sceneBuffer offset:0 atIndex:0];
  [encoder setBuffer:paramsBuffer offset:0 atIndex:1];

  // Configure thread groups
  NSUInteger threadGroupSize = pipelineState.maxTotalThreadsPerThreadgroup;
  NSUInteger threadGroupWidth = 16;
  NSUInteger threadGroupHeight = threadGroupSize / threadGroupWidth;
  MTLSize threadsPerThreadgroup = MTLSizeMake(threadGroupWidth, threadGroupHeight, 1);
  MTLSize threadsPerGrid = MTLSizeMake(width, height, 1);

  std::cout << "Thread group size: " << threadGroupSize << std::endl;
  std::cout << "Thread group dimensions: " << threadGroupWidth << "x" << threadGroupHeight
            << std::endl;
  std::cout << "Dispatching " << width << "x" << height << " threads" << std::endl;

  // Dispatch compute kernel
  [encoder dispatchThreads:threadsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];
  [encoder endEncoding];

  // Execute
  auto start = std::chrono::high_resolution_clock::now();
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
  auto end = std::chrono::high_resolution_clock::now();

  // Read back results
  std::vector<float> textureData(width * height * 4);
  [outputTexture getBytes:textureData.data()
              bytesPerRow:width * 4 * sizeof(float)
               fromRegion:MTLRegionMake2D(0, 0, width, height)
              mipmapLevel:0];

  // Debug: Print first few pixels
  std::cout << "First 10 pixels from Metal GPU:" << std::endl;
  for (int i = 0; i < 10; i++) {
    int idx = i * 4;
    std::cout << "  GPU Pixel " << i << ": (" << textureData[idx] << ", " << textureData[idx + 1]
              << ", " << textureData[idx + 2] << ", " << textureData[idx + 3] << ")" << std::endl;
  }

  // Convert to Color objects and write to file
  std::vector<Color> pixels;
  pixels.reserve(width * height);

  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      uint32_t idx = (y * width + x) * 4;
      Color pixel(textureData[idx], textureData[idx + 1], textureData[idx + 2]);
      pixels.push_back(pixel);
    }
  }

  // Debug: Print first few Color objects
  std::cout << "First 10 Color objects:" << std::endl;
  for (int i = 0; i < 10; i++) {
    std::cout << "  Color " << i << ": (" << pixels[i].r << ", " << pixels[i].g << ", "
              << pixels[i].b << ")" << std::endl;
  }

  // Calculate performance stats
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  uint64_t total_rays = static_cast<uint64_t>(width) * height;
  double rays_per_second = total_rays / (duration.count() / 1000.0);

  std::cout << total_rays << " rays traced in " << duration.count() << " ms at "
            << static_cast<uint64_t>(rays_per_second) << " rays/s using Metal GPU" << std::endl;

  // Write image
  writeImageToPPM(pixels, width, height, output_file);
  std::cout << "Image written to " << output_file << std::endl;
  std::cout << "Render complete! Output saved to: " << output_file << std::endl;

  return 0;
}