#include <metal_stdlib>
using namespace metal;

// Maximum number of objects in the scene
#define MAX_SPHERES 32
#define MAX_TRIANGLES 128
#define MAX_LIGHTS 16
#define MAX_REFLECTIONS 3

// Structure definitions matching C++ side
struct Ray {
    float3 origin;
    float3 direction;
};

struct Sphere {
    float3 center;
    float radius;
    float3 color;
    float reflectivity;
};

struct Triangle {
    float3 v0, v1, v2;
    float3 normal;
    float3 color;
    float reflectivity;
};

struct Light {
    float3 position;
    float3 color;
    float intensity;
    int type; // 0 = point, 1 = area
    float width, height;
    int samples;
    float padding[2]; // Ensure 16-byte alignment to match C++ structure
};

struct Camera {
    float3 position;
    float3 look_at;
    float3 up;
    float fov;
    float aspect_ratio;
    float padding[3]; // Ensure 16-byte alignment to match C++ structure
};

struct SceneData {
    Camera camera;
    int num_spheres;
    int num_triangles;
    int num_lights;
    int padding; // Ensure alignment to match C++ structure
    Sphere spheres[MAX_SPHERES];
    Triangle triangles[MAX_TRIANGLES];
    Light lights[MAX_LIGHTS];
    float3 background_color;
    int samples_per_pixel;
};

struct RenderParams {
    uint width;
    uint height;
    uint samples_per_pixel;
    uint padding; // Ensure 16-byte alignment to match C++ structure
};

// Ray-sphere intersection
bool intersect_sphere(Ray ray, Sphere sphere, thread float& t) {
    float3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) return false;
    
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
    
    t = (t1 > 0.001) ? t1 : t2;
    return t > 0.001;
}

// Ray-triangle intersection using Möller-Trumbore algorithm
bool intersect_triangle(Ray ray, Triangle tri, thread float& t) {
    float3 edge1 = tri.v1 - tri.v0;
    float3 edge2 = tri.v2 - tri.v0;
    float3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    if (a > -0.00001 && a < 0.00001) return false;
    
    float f = 1.0 / a;
    float3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    if (u < 0.0 || u > 1.0) return false;
    
    float3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    if (v < 0.0 || u + v > 1.0) return false;
    
    t = f * dot(edge2, q);
    return t > 0.001;
}

// Generate camera ray
Ray generate_camera_ray(uint2 pixel, RenderParams params, Camera camera, float2 sample_offset) {
    // Convert pixel coordinates to normalized device coordinates [-1, 1]
    float2 ndc = (float2(pixel) + sample_offset) / float2(params.width, params.height);
    ndc = ndc * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y coordinate (screen space to camera space)
    
    // Calculate camera parameters
    float theta = camera.fov * M_PI_F / 180.0;
    float half_height = tan(theta * 0.5);
    float half_width = camera.aspect_ratio * half_height;
    
    // Build camera coordinate system
    float3 forward = normalize(camera.look_at - camera.position); // Camera forward direction
    float3 right = normalize(cross(forward, camera.up));          // Camera right direction  
    float3 up = cross(right, forward);                            // Camera up direction
    
    // Calculate ray direction in world space
    float3 ray_dir = normalize(
        ndc.x * half_width * right +   // Horizontal component
        ndc.y * half_height * up +     // Vertical component
        forward                        // Forward component
    );
    
    Ray ray;
    ray.origin = camera.position;
    ray.direction = ray_dir;
    
    return ray;
}

// Simple pseudorandom number generator
float random(thread uint& seed) {
    seed = seed * 1103515245 + 12345;
    return float(seed) / 4294967296.0;
}

float2 random2(thread uint& seed) {
    return float2(random(seed), random(seed));
}

// Trace ray and find closest intersection
float3 trace_ray(Ray ray, constant SceneData& scene, thread uint& seed, int depth) {
    if (depth >= MAX_REFLECTIONS) {
        return scene.background_color;
    }
    
    float closest_t = INFINITY;
    float3 hit_color = scene.background_color;
    float3 hit_point, hit_normal;
    float reflectivity = 0.0;
    bool hit = false;
    
    // Test sphere intersections
    for (int i = 0; i < scene.num_spheres; i++) {
        float t;
        if (intersect_sphere(ray, scene.spheres[i], t) && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + t * ray.direction;
            hit_normal = normalize(hit_point - scene.spheres[i].center);
            hit_color = scene.spheres[i].color;
            reflectivity = scene.spheres[i].reflectivity;
            hit = true;
        }
    }
    
    // Test triangle intersections
    for (int i = 0; i < scene.num_triangles; i++) {
        float t;
        if (intersect_triangle(ray, scene.triangles[i], t) && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + t * ray.direction;
            hit_normal = scene.triangles[i].normal;
            hit_color = scene.triangles[i].color;
            reflectivity = scene.triangles[i].reflectivity;
            hit = true;
        }
    }
    
    if (!hit) {
        return scene.background_color;
    }
    
    // Calculate lighting
    float3 final_color = float3(0.0);
    float3 ambient = hit_color * 0.1;
    
    // Process all lights
    for (int i = 0; i < scene.num_lights; i++) {
        Light light = scene.lights[i];
        
        if (light.type == 0) {
            // Point light
            float3 light_dir = normalize(light.position - hit_point);
            float3 diffuse = max(0.0, dot(hit_normal, light_dir)) * hit_color * light.color * light.intensity;
            final_color += diffuse;
        } else {
            // Area light with soft shadows
            float3 total_diffuse = float3(0.0);
            int samples = max(1, light.samples);
            
            for (int s = 0; s < samples; s++) {
                float2 offset = (random2(seed) - 0.5) * float2(light.width, light.height);
                float3 sample_pos = light.position + float3(offset.x, 0.0, offset.y);
                float3 light_dir = normalize(sample_pos - hit_point);
                float3 diffuse = max(0.0, dot(hit_normal, light_dir)) * hit_color * light.color * light.intensity;
                total_diffuse += diffuse;
            }
            final_color += total_diffuse / float(samples);
        }
    }
    
    final_color += ambient;
    
    // Handle reflections
    if (reflectivity > 0.0 && depth < MAX_REFLECTIONS) {
        float3 reflect_dir = reflect(ray.direction, hit_normal);
        Ray reflect_ray;
        reflect_ray.origin = hit_point + hit_normal * 0.001; // Offset to avoid self-intersection
        reflect_ray.direction = reflect_dir;
        
        float3 reflected_color = trace_ray(reflect_ray, scene, seed, depth + 1);
        final_color = mix(final_color, reflected_color, reflectivity);
    }
    
    return final_color;
}

// Main compute shader for ray tracing
kernel void raytracer_kernel(
    texture2d<float, access::write> output_texture [[texture(0)]],
    constant SceneData& scene [[buffer(0)]],
    constant RenderParams& params [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.width || gid.y >= params.height) {
        return;
    }
    
    // Initialize random seed based on pixel coordinates
    uint seed = gid.x + gid.y * params.width + 1;
    
    float3 pixel_color = float3(0.0);
    
    // Generate ray from camera
    float2 sample_offset = float2(0.5); // Center of pixel
    Ray ray = generate_camera_ray(gid, params, scene.camera, sample_offset);
    
    float closest_t = INFINITY;
    float3 hit_color = scene.background_color;
    
    // Test intersection with all spheres - assign unique colors
    for (int i = 0; i < min(scene.num_spheres, MAX_SPHERES); i++) {
        Sphere sphere = scene.spheres[i];
        float t;
        if (intersect_sphere(ray, sphere, t)) {
            if (t < closest_t) {
                closest_t = t;
                // Assign unique colors for spheres: red, green, blue, etc.
                if (i == 0) hit_color = float3(1.0, 0.0, 0.0);      // Red
                else if (i == 1) hit_color = float3(0.0, 1.0, 0.0); // Green  
                else if (i == 2) hit_color = float3(0.0, 0.0, 1.0); // Blue
                else hit_color = float3(1.0, 1.0, 0.0);             // Yellow for others
            }
        }
    }
    
    // Test intersection with all triangles - assign unique colors per group
    for (int i = 0; i < min(scene.num_triangles, MAX_TRIANGLES); i++) {
        Triangle triangle = scene.triangles[i];
        float t;
        if (intersect_triangle(ray, triangle, t)) {
            if (t < closest_t) {
                closest_t = t;
                // Assign colors based on triangle groups (12 triangles per box typically)
                int group = i / 12; // Each box has 12 triangles
                if (group == 0) hit_color = float3(1.0, 0.0, 1.0);      // Magenta - Left wall
                else if (group == 1) hit_color = float3(0.0, 1.0, 1.0); // Cyan - Right wall
                else if (group == 2) hit_color = float3(0.7, 0.7, 0.7); // Gray - Floor
                else if (group == 3) hit_color = float3(0.9, 0.9, 0.9); // Light gray - Ceiling
                else if (group == 4) hit_color = float3(0.5, 0.5, 0.5); // Dark gray - Back wall
                else if (group == 5) hit_color = float3(1.0, 0.5, 0.0); // Orange - Box 1
                else if (group == 6) hit_color = float3(0.5, 0.0, 1.0); // Purple - Box 2
                else hit_color = float3(1.0, 1.0, 1.0);                 // White for others
            }
        }
    }
    
    pixel_color = hit_color;
    
    output_texture.write(float4(pixel_color, 1.0), gid);
}