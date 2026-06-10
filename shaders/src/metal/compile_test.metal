#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float4 position;
};

struct VertexOut {
    float4 position [[position]];
};

vertex VertexOut vertexShader(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = in.position;
    return out;
}
