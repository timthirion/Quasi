#!/usr/bin/env python3
import json
import sys
import os

def analyze_mesh(filename):
    """Analyze a mesh file and print detailed statistics."""
    try:
        with open(filename) as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        return False
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in '{filename}': {e}")
        return False
    
    # Extract mesh name for display
    mesh_name = data.get('name', os.path.splitext(os.path.basename(filename))[0])
    
    print(f"=== Mesh Analysis: {mesh_name} ===")
    print(f"File: {filename}")
    
    # Check for both compact and legacy formats
    if 'vertices' in data and 'indices' in data:
        # Compact vertex/index format
        vertices = data['vertices']
        indices = data['indices']
        
        if len(vertices) % 3 != 0:
            print(f"Error: Vertex array length ({len(vertices)}) is not divisible by 3")
            return False
            
        if len(indices) % 3 != 0:
            print(f"Error: Index array length ({len(indices)}) is not divisible by 3")
            return False
        
        vertex_count = len(vertices) // 3
        triangle_count = len(indices) // 3
        
        print(f"Format: Compact (vertex/index buffers)")
        print(f"Vertices: {vertex_count}")
        print(f"Triangles: {triangle_count}")
        
        # Convert to (x,y,z) tuples for analysis
        vertex_list = []
        for i in range(0, len(vertices), 3):
            vertex_list.append((vertices[i], vertices[i+1], vertices[i+2]))
            
    elif 'triangles' in data:
        # Legacy explicit triangle format
        triangles = data['triangles']
        triangle_count = len(triangles)
        vertex_count = triangle_count * 3  # Not unique vertices
        
        print(f"Format: Legacy (explicit triangles)")
        print(f"Triangles: {triangle_count}")
        print(f"Vertex entries: {vertex_count} (may include duplicates)")
        
        # Extract all vertices from triangles
        vertex_list = []
        for triangle in triangles:
            for vertex_key in ['v0', 'v1', 'v2']:
                v = triangle[vertex_key]
                vertex_list.append((v[0], v[1], v[2]))
    else:
        print("Error: No vertex data found. Expected 'vertices'+'indices' or 'triangles' format.")
        return False
    
    if not vertex_list:
        print("Error: No vertices found in mesh.")
        return False
    
    # Calculate bounds
    min_x = min(v[0] for v in vertex_list)
    max_x = max(v[0] for v in vertex_list)
    min_y = min(v[1] for v in vertex_list)
    max_y = max(v[1] for v in vertex_list)
    min_z = min(v[2] for v in vertex_list)
    max_z = max(v[2] for v in vertex_list)
    
    center_x = (min_x + max_x) / 2
    center_y = (min_y + max_y) / 2
    center_z = (min_z + max_z) / 2
    
    size_x = max_x - min_x
    size_y = max_y - min_y
    size_z = max_z - min_z
    
    print(f"\nBounding Box:")
    print(f"  X: {min_x:.4f} to {max_x:.4f} (size: {size_x:.4f})")
    print(f"  Y: {min_y:.4f} to {max_y:.4f} (size: {size_y:.4f})")
    print(f"  Z: {min_z:.4f} to {max_z:.4f} (size: {size_z:.4f})")
    print(f"Center: ({center_x:.4f}, {center_y:.4f}, {center_z:.4f})")
    print(f"Max dimension: {max(size_x, size_y, size_z):.4f}")
    
    # Additional metadata
    if 'scale' in data:
        print(f"Default scale: {data['scale']}")
    if 'center' in data:
        default_center = data['center']
        print(f"Default center: ({default_center[0]:.4f}, {default_center[1]:.4f}, {default_center[2]:.4f})")
    
    return True

def print_usage():
    print("Usage: python3 analyze_mesh.py <mesh_file.json>")
    print("       python3 analyze_mesh.py data/models/bunny.json")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print_usage()
        sys.exit(1)
    
    filename = sys.argv[1]
    if not analyze_mesh(filename):
        sys.exit(1)