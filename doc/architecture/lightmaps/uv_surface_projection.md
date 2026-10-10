# Projecting surface geometry into the UV atlas

## What is projected

`UvSurfacePass` creates a surface lookup image: for each covered UV2 texel it
stores a world position and world normal from which a lighting ray can start.
It rasterizes the original indexed geometry with UV2 as the screen position.
It does not render the object through a camera and then copy that image to an
atlas, and it does not project illumination from a light's view. The atlas
parameterization replaces the camera's view/projection transformation.

The pass is internal to the baker. `LightmapBaker::recordUvSurface()` obtains
the target's directly attached Drawable and `Node::worldMatrix()`. The same
pass runs each sample, so surface attributes reflect the supplied transforms;
accumulation still requires those inputs to stay consistent between samples.
One `GBufferManager` exists per target and engine frame slot.

Sources: [LightmapBaker](../../../lib/src/bg2e/render/LightmapBaker.cpp),
[UvSurfacePass header](../../../lib/src/bg2e/render/UvSurfacePass.hpp),
[implementation](../../../lib/src/bg2e/render/UvSurfacePass.cpp).

## Two coordinate paths through one vertex shader

For submesh \(s\), the CPU supplies:

\[
M_s=M_{nodeWorld}\,M_{drawable}\,M_{localSubmesh,s}.
\]

The order matches the actual draw path. A position becomes
\(P_{world}=M_s(P_{object},1)\). The vertex normal uses the inverse transpose
of the upper-left 3×3 matrix, then normalization. Using the position matrix
for a normal would be incorrect under nonuniform scaling. Singular transforms
make this inverse undefined; the pass has no special recovery for that case.

```cpp
// UvSurfacePass::record(), before drawSubmesh().
const glm::mat4 objectToWorld = nodeWorld * drawable.transform() *
    drawable.localSubmeshTransform(submeshIndex);
vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
    0, sizeof(glm::mat4), &objectToWorld);
vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
    sizeof(glm::mat4), sizeof(uint32_t), &submeshIndex);
renderMesh->drawSubmesh(cmd, submeshIndex);
```

Source: [UvSurfacePass.cpp](../../../lib/src/bg2e/render/UvSurfacePass.cpp).
The packed push-constant range contains one matrix and a UInt32 identity.
The vertex input is the standard Drawable layout: position, normal, UV1, UV2.
Tangents are not needed for this geometry-only projection.

The second path determines rasterization coordinates directly from UV2:

```glsl
vec4 position = drawData.objectToWorld * vec4(inPosition, 1.0);
worldPosition = position.xyz;
worldNormal = normalize(transpose(inverse(mat3(drawData.objectToWorld))) * inNormal);
submeshIndex = drawData.submeshIndex;
sourceUv1 = inUv1;

vec2 uv2Inv = vec2(inUv2.x, 1.0 - inUv2.y);
gl_Position = vec4(uv2Inv * 2.0 - 1.0, 0.0, 1.0);
```

Source: [uv_surface.vert.glsl](../../../shaders/src/glsl/uv_surface.vert.glsl).
Thus clip X is `2u-1`, clip Y is `1-2v`, Z is zero and W is one. World position
never enters `gl_Position`; translating the target changes stored positions
without moving its atlas triangles. Every vertex has the same W, so
perspective-correct interpolation reduces to ordinary affine interpolation
within the UV triangle.

## Viewport orientation and pixel centers

The engine sets `viewport.y = H`, `viewport.height = -H`, width `W` and x zero.
Combining those assignments with the shader above gives framebuffer coordinates:

\[
x_{fb}=Wu,\qquad
 y_{fb}=H-\tfrac H2((1-2v)+1)=Hv.
\]

Consequently the first framebuffer row is near `v=0`, and row `j` has center
`v=(j+0.5)/H`; column `i` has center `u=(i+0.5)/W`. Both the explicit shader
V inversion and the negative viewport height must be considered. Source comments
say that `v=1` is at the top, but the executed expressions compose as shown above.
Readback copies rows directly; it does not apply a CPU UV inversion. The preview
shader uses the same mapping, making the preview and surface rasterization agree.

Sources: [viewport helper](../../../lib/src/bg2e/render/vulkan/macros/graphics.cpp),
[surface shader](../../../shaders/src/glsl/uv_surface.vert.glsl),
[preview shader](../../../shaders/src/glsl/uv_map_preview.vert.glsl),
[readback](../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp).
This orientation follows the source expressions; it has not been established
by a new runtime render test.

## From a covered texel back to the surface

At the center of a covered texel, rasterization derives barycentric weights
\(\lambda_0,\lambda_1,\lambda_2\) in the UV triangle. Their sum is one and the
interpolated world position is:

\[
P(i,j)=\lambda_0 P_0^{world}+\lambda_1 P_1^{world}+\lambda_2 P_2^{world}.
\]

The world normal is the normalized interpolation of the already transformed,
normalized vertex normals. It is a smooth geometry normal, not necessarily the
triangle face normal; no normal texture is sampled at the initial bake surface.
`sourceUv1` is also interpolated, but the current fragment shader does not store
or sample it. Initial texel coverage is therefore geometry-only: no albedo,
alpha or material texture changes the UV surface coverage. Alpha cutouts are
handled later at ray intersections, a separate responsibility.

The submesh index is declared `flat`, so an integer material identity is retained
without interpolation. Because each draw uses one push-constant identity, every
fragment of that draw gets the same submesh index.

```glsl
outWorldPosition = vec4(worldPosition, 1.0);
outWorldNormal = vec4(normalize(worldNormal), 0.0);
outSubmeshIndex = submeshIndex;
outValidTexel = 1u;
```

Source: [uv_surface.frag.glsl](../../../shaders/src/glsl/uv_surface.frag.glsl).
The actual coverage follows standard single-sample triangle rasterization;
there is no multisampling or conservative rasterization. Thin charts or very
small UV triangles can have no sampled texel even when geometrically valid.

## Depthless attachment profile

| Index | Format | Stored information |
|---|---|---|
| 0 | `R32G32B32A32_SFLOAT` | World position; alpha 1 for rasterized fragments. |
| 1 | `R16G16B16A16_SFLOAT` | Signed world normal; alpha zero. |
| 2 | `R32_UINT` | Zero-based submesh identity. |
| 3 | `R32_UINT` | Coverage: zero for clear/background, one for a fragment. |

`depthFormat = VK_FORMAT_UNDEFINED`: the configurable G-buffer allocates no
depth image. The graphics pipeline disables depth testing and multisampling;
its factory defaults also disable culling and blending. Triangle orientation
in UV space therefore does not remove coverage. All submesh ranges are drawn
by this pass; it does not explicitly filter submesh visibility here.

`GBufferManager::beginRender(cmd)` clears every attachment to zero, transitions
it to color-attachment use and begins Vulkan dynamic rendering. No `VkRenderPass`
object is required. After drawing, `cmdEndRendering()` and
`transitionToShaderRead()` make the buffers available to compute/ray shaders.
The manager validates image dimensions, attachment count and device format
support when building. Its default camera profile differs from this explicit
four-attachment profile.

An uncovered texel's submesh ID is zero, which is also a valid first submesh.
Consumers must use attachment 3 to distinguish coverage; neither ID zero nor a
world position at the origin is a reliable background marker. The current
lighting passes consume position, normal and mask, not the identity attachment.
Identity remains available for diagnostics and future surface-material use.

Sources: [profile/pipeline](../../../lib/src/bg2e/render/UvSurfacePass.cpp),
[GBufferManager](../../../lib/src/bg2e/render/gbuffer/GBufferManager.cpp),
[graphics defaults](../../../lib/src/bg2e/render/vulkan/factory/GraphicsPipeline.cpp).

## Why atlas validation is a prerequisite

No camera depth ordering can choose which surface should own an overlapping UV
texel. With depth and blending disabled, overlapping triangles would overwrite
one another according to drawing; that is not a valid surface parameterization.
`LightmapBaker::validateTarget()` therefore requires the CPU atlas validator
before creating the pass. Shared chart edges are allowed, while overlapping
positive-area interiors are rejected. Collapsed triangles cover no texels.
The UV rasterizer itself is not an overlap validator.

The internal `UvSurfacePass::Diagnostic` returns extent plus UInt32 submesh-ID
and valid-texel arrays. `readDiagnostic(slot)` waits for device idle, copies both
images through a staging buffer, invalidates the mapped allocation and restores
shader-read layouts. It must be used after submitting the recorded frame;
waiting for device idle cannot submit an unfinished command buffer. This is an
internal debugging facility, not a public baker method.

The next stage reads each valid texel's surface point and queries the complete
scene. See [lighting architecture](lighting_passes.md) and
[stable atlas accumulation](accumulation.md).
