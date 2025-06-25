# Scene Collection

This directory contains test scenes for the Quasi ray tracer, showcasing different rendering features including basic geometry, lighting, tone mapping, and the Cornell Box.

## Core Scenes

### `cornell_box.json`
- **Standard Cornell Box scene** with traditional lighting levels and improved tone mapping
- Red left wall, green right wall, white ceiling/floor/back wall
- Two spheres with varying reflectance for subtle tone mapping benefits
- Point light source with intensity 1.2 (traditional Cornell Box lighting)
- Demonstrates: box primitives, triangle rendering, Phong lighting, natural HDR tone mapping
- Resolution: 512x512, 4 samples per pixel

### `cornell_box_showcase.json`
- **High-quality Cornell Box showcase** with multiple objects and dual lighting
- **Resolution: 1024x1024** with adaptive sampling (8-128 samples per pixel)
- Three highly reflective spheres with balanced dual-light setup
- Conservative lighting: 1.0 and 0.8 intensity point lights for natural appearance
- Demonstrates: advanced multisampling, tone mapping preserving natural contrast, complex reflections

### `cornell_box_tone_mapping_demo.json`
- **Enhanced Cornell Box** - showcases tone mapping benefits with moderate lighting
- **Ultra-high quality**: 16-256 adaptive samples with variance threshold 0.001
- Triple lighting setup with intensities 1.4, 1.3, and 1.0 (subtle HDR values)
- Highly reflective spheres (0.95 and 0.85 reflectance) for controlled bright highlights
- Perfect for seeing tone mapping benefits while maintaining natural exposure levels

### `basic_lighting_test.json`
- **Simple lighting test** with single sphere and point light
- White sphere with point light for debugging lighting calculations
- Minimal scene for testing Phong lighting implementation
- Resolution: 128x128

## Test & Demo Scenes

### `default_scene.json`
- Simple sphere with checkerboard background
- Good for basic functionality testing without lighting

### `sphere_collection.json`
- Multiple spheres of different sizes and colors
- Tests sphere rendering and intersection

### `sphere_test_scene.json`
- Single sphere test scene
- Minimal scene for debugging

### `five_spheres.json`
- Five spheres arranged in a pattern
- Tests multiple object rendering

### `multisampling_test_scene.json`
- **Multisampling demonstration scene** with three colored spheres
- Resolution: 400x400 with 4 samples per pixel
- Features stratified sampling and average integration
- Good for testing multisampling performance and quality

### `high_quality_test.json`
- **High sample count test** with single red sphere
- Resolution: 200x200 with **16 samples per pixel**
- Demonstrates maximum quality rendering with heavy antialiasing

### `test_scene.json`
- Simple test scene for development and debugging
- Basic sphere and lighting setup

## Features Demonstrated

- **Sphere primitives** - Ray-sphere intersection
- **Box primitives** - Decomposed into 12 triangles (6 faces × 2 triangles)
- **Triangle rendering** - Ray-triangle intersection using Möller-Trumbore algorithm
- **Phong lighting** - Ambient + diffuse + specular components with HDR support
- **Point lights** - Position-based illumination with intensity control (supports HDR values > 1.0)
- **Material system** - Solid color materials with reflectance properties
- **Cornell Box** - Standard computer graphics test scene with HDR enhancements
- **Multisampling anti-aliasing** - Stratified sampling with configurable sample counts
- **Extensible sampling system** - Support for different sampling patterns and integrators
- **HDR tone mapping** - Reinhard, ACES, and exposure-based tone mapping operators
- **Professional color pipeline** - HDR color processing with gamma correction
- **Bright highlight preservation** - No HDR clamping, preserves specular highlights

## Usage

To render a scene:
```bash
./quasi-build/src/apps/rt <scene_file.json> <output.ppm>
```

**Note**: All scenes now use HDR tone mapping by default (Reinhard operator with 2.2 gamma correction).

### Recommended Examples:

```bash
# Standard Cornell Box with HDR tone mapping
./quasi-build/src/apps/rt scenes/cornell_box.json cornell_box.ppm

# High-quality showcase with complex reflections  
./quasi-build/src/apps/rt scenes/cornell_box_showcase.json showcase.ppm

# Tone mapping demonstration with extreme HDR values
./quasi-build/src/apps/rt scenes/cornell_box_tone_mapping_demo.json tone_mapping_demo.ppm

# Test basic lighting
./quasi-build/src/apps/rt scenes/basic_lighting_test.json lighting_test.ppm
```

### Tone Mapping Benefits:
- **No blown highlights** - Bright specular reflections are preserved and smoothly compressed
- **Enhanced contrast** - Better separation between dark and bright areas  
- **Professional quality** - Images look more natural and film-like
- **HDR support** - Light intensities > 1.0 produce realistic bright lighting effects