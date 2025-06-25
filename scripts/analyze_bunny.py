#!/usr/bin/env python3
import json
import sys

def analyze_bunny():
    with open('data/models/bunny.json') as f:
        data = json.load(f)
    
    vertices = data['vertices']
    
    # Convert to (x,y,z) tuples
    vertex_list = []
    for i in range(0, len(vertices), 3):
        vertex_list.append((vertices[i], vertices[i+1], vertices[i+2]))
    
    # Find bounds
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
    
    print(f"Bunny bounds:")
    print(f"  X: {min_x:.4f} to {max_x:.4f} (size: {size_x:.4f})")
    print(f"  Y: {min_y:.4f} to {max_y:.4f} (size: {size_y:.4f})")
    print(f"  Z: {min_z:.4f} to {max_z:.4f} (size: {size_z:.4f})")
    print(f"Center: ({center_x:.4f}, {center_y:.4f}, {center_z:.4f})")
    print(f"Max dimension: {max(size_x, size_y, size_z):.4f}")

if __name__ == "__main__":
    analyze_bunny()