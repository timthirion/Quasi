# Scene Files

This directory contains JSON scene files for the Quasi raytracer.

## Available Scenes

### `default_scene.json`
The original three-sphere scene that was hardcoded in rt.cpp, now defined in JSON format.
- 3 spheres (red, green, blue)
- 800x600 resolution
- 6x8 checkerboard background

### `five_spheres.json`
A more complex scene demonstrating the flexibility of the JSON format.
- 5 spheres of varying sizes and colors
- 1200x800 resolution
- Elevated camera position
- 10x12 checkerboard background

### `sphere_collection.json`
A collection of 6 spheres with mixed sizes and colors.
- 800x800 square resolution
- Dark checkerboard background
- Various sphere positions and colors

### `sphere_test_scene.json`
A 5-sphere test scene with traditional positioning.
- 600x600 square resolution
- Conservative camera setup

## Limitations

**Note**: The current raytracer only supports spheres and checkerboard backgrounds. To implement the actual Cornell Box dataset, we would need to add support for:
- Box/cuboid primitives
- Room geometry (planes/walls)
- Material properties (diffuse colors for walls)
- Area lighting
- More sophisticated lighting model

The Cornell Box is a specific computer graphics test scene consisting of a room with two cuboid boxes inside, not spheres.

## Usage

```bash
./rt scenes/scene_name.json
```