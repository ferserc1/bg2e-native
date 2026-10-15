#include "vertex.metal"
vertex RasterVaryings vertMain(RasterVertex in [[stage_in]], DRAW_VERTEX_DATA_ARGUMENTS)
{
    RasterVaryings out;
    out.worldPosition = (drawObject.model * float4(in.position, 1.0)).xyz;
    out.worldNormal = normalize(float3x3(drawObject.normal[0].xyz, drawObject.normal[1].xyz, drawObject.normal[2].xyz) * in.normal);
    float3 t = float3x3(drawObject.model[0].xyz, drawObject.model[1].xyz, drawObject.model[2].xyz) * in.tangent;
    t -= out.worldNormal * dot(t, out.worldNormal);
    if (dot(t, t) < 1e-8)
    {
        t = cross(abs(out.worldNormal.z) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0), out.worldNormal);
    }
    out.worldTangent = normalize(t);
    out.tangentSign = determinant(float3x3(drawObject.model[0].xyz, drawObject.model[1].xyz, drawObject.model[2].xyz)) < 0.0 ? -1.0 : 1.0;
    out.uv0 = in.texcoord0;
    out.uv1 = in.texcoord1;
    out.position = drawFrame.projection * drawFrame.view * float4(out.worldPosition, 1.0);
    return out;
}
