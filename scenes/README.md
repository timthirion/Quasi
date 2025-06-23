# Scene Collection

This directory contains test scenes for the Quasi ray tracer, showcasing different rendering features including basic geometry, lighting, and the Cornell Box.

## Core Scenes

### `cornell_box.json`
- **Complete Cornell Box scene** with Phong lighting
- Red left wall, green right wall, white ceiling/floor/back wall
- White sphere and two white rectangular boxes
- Point light source at (0, 0.8, 0) with intensity 2.0
- Demonstrates: box primitives, triangle rendering, Phong lighting, point lights
- Resolution: 256x256

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

## Features Demonstrated

- **Sphere primitives** - Ray-sphere intersection
- **Box primitives** - Decomposed into 12 triangles (6 faces × 2 triangles)
- **Triangle rendering** - Ray-triangle intersection using Möller-Trumbore algorithm
- **Phong lighting** - Ambient + diffuse + specular components
- **Point lights** - Position-based illumination with intensity control
- **Material system** - Solid color materials with lighting properties
- **Cornell Box** - Standard computer graphics test scene

## Usage

To render a scene:
```bash
./quasi-build/src/apps/rt <scene_file.json> <output.ppm>
```

Examples:
```bash
# Render Cornell Box with lighting
./quasi-build/src/apps/rt scenes/cornell_box.json cornell_box.ppm

# Test basic lighting
./quasi-build/src/apps/rt scenes/basic_lighting_test.json lighting_test.ppm

# Simple sphere test
./quasi-build/src/apps/rt scenes/default_scene.json basic_test.ppm
```