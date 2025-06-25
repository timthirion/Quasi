#!/usr/bin/env python3
import json
import sys

def convert_obj_to_json(obj_file, json_file):
    vertices = []
    indices = []
    vertex_count = 0
    face_count = 0
    
    with open(obj_file, 'r') as f:
        lines = f.readlines()
    
    # Parse header info
    for line in lines[:10]:  # Check first 10 lines for metadata
        if line.startswith('# vertex count ='):
            vertex_count = int(line.split('=')[1].strip())
        elif line.startswith('# face count ='):
            face_count = int(line.split('=')[1].strip())
    
    # Parse vertices
    for line in lines:
        if line.startswith('v '):
            parts = line.strip().split()
            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
            vertices.extend([x, y, z])
    
    # Parse faces (convert from 1-based to 0-based indexing)
    for line in lines:
        if line.startswith('f '):
            parts = line.strip().split()
            # OBJ uses 1-based indexing, convert to 0-based
            v1, v2, v3 = int(parts[1]) - 1, int(parts[2]) - 1, int(parts[3]) - 1
            indices.extend([v1, v2, v3])
    
    # Create JSON data
    json_data = {
        "name": "Stanford Bunny (OBJ Converted)",
        "description": f"Stanford bunny model converted from OBJ format - {vertex_count} vertices, {face_count} faces",
        "vertex_count": vertex_count,
        "face_count": face_count,
        "vertices": vertices,
        "indices": indices
    }
    
    # Verify counts
    actual_vertices = len(vertices) // 3
    actual_faces = len(indices) // 3
    
    print(f"Expected: {vertex_count} vertices, {face_count} faces")
    print(f"Actual: {actual_vertices} vertices, {actual_faces} faces")
    print(f"Vertices array length: {len(vertices)}")
    print(f"Indices array length: {len(indices)}")
    
    if actual_vertices != vertex_count:
        print(f"WARNING: Vertex count mismatch!")
    if actual_faces != face_count:
        print(f"WARNING: Face count mismatch!")
    
    # Write JSON file
    with open(json_file, 'w') as f:
        json.dump(json_data, f, indent=2)
    
    print(f"Converted {obj_file} to {json_file}")

if __name__ == "__main__":
    convert_obj_to_json("bunny.obj", "data/models/bunny.json")