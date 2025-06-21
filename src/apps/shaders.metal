#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 color;
};

vertex VertexOut vertex_main(const device VertexIn* vertices [[buffer(0)]],
                            uint vertexId [[vertex_id]]) {
    VertexOut out;
    VertexIn in = vertices[vertexId];
    
    out.position = float4(in.position, 1.0);
    out.color = in.color;
    
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}