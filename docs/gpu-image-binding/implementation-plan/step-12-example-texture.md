# Step 12 — Example: 2×2 procedural texture + sampler + textured triangle (Milestone 2)

## Title
Sample a procedural 2×2 texture in the triangle's fragment shader.

## Objective
Complete the example: create a 2×2 procedural RGBA8 texture and a sampler, bind
them to the graphics pipeline via a `ResourceSet`, add UVs to the triangle, and
sample the texture in the fragment shader — on both Vulkan and Metal. **This is
the final milestone.**

## Context
All engine APIs now exist: `createImage` (Step 10), `uploadRGBA8` (Step 11),
`createSampler` (Step 03), `ResourceSet` + `bindResourceSet` (Steps 06–07), and
`PipelineLayout` resource bindings (Step 05). Only the example changes here.

## Expected previous state
- Steps 01–11 complete. Gradient milestone working; image create/upload and
  sampler APIs available.

## Files to review / modify
- Review: `examples/gpu/05_simple_triangle/src/main.cpp`,
  `shaders/glsl/triangle.vert.glsl`, `shaders/glsl/triangle.frag.glsl`,
  `shaders/metal/triangle.metal`.
- Modify (example only): the three shader files above and `src/main.cpp`.

## Proposed design
### Procedural texture (example C++)
```cpp
const std::array<std::array<uint8_t,4>, 4> texels = {{
    {{255,   0,   0, 255}}, {{  0, 255,   0, 255}},
    {{  0,   0, 255, 255}}, {{255, 255,   0, 255}}
}};
auto texture = device->createImage({ {2,2}, gpu::PixelFormat::R8G8B8A8_UNORM,
                                     gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst });
texture->uploadRGBA8(texels.data(), { 2, 2 });

auto sampler = device->createSampler({}); // default: linear/linear, repeat
```

### Graphics layout + resource set (example C++)
Separate from the compute layout. Use **separate `SampledImage` + `Sampler`**
bindings (design decision in the overview):
```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
graphicsLayoutDesc.resourceBindings.push_back({ 0, 0, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1 });
graphicsLayoutDesc.resourceBindings.push_back({ 0, 1, gpu::ResourceType::Sampler,      gpu::ShaderStage::Fragment, 1 });
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);

// Static set (texture + sampler never change) — created once:
auto textureSet = device->createResourceSet(graphicsLayout.get(), 0);
textureSet->setSampledImage(0, texture.get());
textureSet->setSampler(1, sampler.get());
textureSet->update();
```
Per frame, inside rendering, after `bindPipeline(trianglePipeline)`:
```cpp
cmd->bindResourceSet(trianglePipeline.get(), 0, textureSet.get());
```

### Shaders (example only)
GLSL `triangle.vert.glsl`: add a UV output.
```glsl
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
// uvs[3] = { (0.5,1.0), (1.0,0.0), (0.0,0.0) } per vertex
```
GLSL `triangle.frag.glsl`: separate texture + sampler.
```glsl
layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler   uSampler;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(push_constant) uniform Push { vec4 color; } pc;
layout(location = 0) out vec4 outColor;
void main() {
    vec4 tex = texture(sampler2D(uTex, uSampler), fragUV);
    outColor = vec4(tex.rgb * fragColor * pc.color.rgb, tex.a * pc.color.a);
}
```
Metal `triangle.metal`: add UV to `VertexOut`, and sample with separate texture +
sampler at the matching indices:
```metal
fragment float4 triangle_fragment(VertexOut in [[stage_in]],
                                  constant PushConstants& pc [[buffer(0)]],
                                  texture2d<float> uTex      [[texture(0)]],
                                  sampler          uSampler  [[sampler(1)]])
{
    float4 tex = uTex.sample(uSampler, in.uv);
    return float4(tex.rgb * in.color * pc.color.rgb, tex.a * pc.color.a);
}
```

### Cleanup
Release `textureSet`, `sampler`, `texture`, and `graphicsLayout` (now with
bindings) alongside existing cleanup.

## Compilation criteria
- Example compiles on both backends; new/edited shaders build via existing
  `CMakeLists.txt` rules (auto-discovered).

## Validation criteria
- The triangle shows the 2×2 texture (four colored quadrants, bilinearly blended)
  modulated by the per-vertex color and the animated push-constant color, over the
  compute gradient background.
- No Vulkan validation errors (descriptor types `SAMPLED_IMAGE` + `SAMPLER`, image
  in `SHADER_READ_ONLY_OPTIMAL` at draw).
- No Metal validation errors (texture at index 0, sampler at index 1, fragment
  stage).
- Both backends produce visually equivalent results.

## Risks / things to check
- **Binding-index agreement:** the GLSL `binding = 0/1` and the Metal
  `[[texture(0)]]` / `[[sampler(1)]]` must match the `ResourceBinding.binding`
  values and the Metal index mapping from Step 05 (`index = binding`). Texture and
  sampler live in separate Metal index namespaces, so binding 0 (texture) and
  binding 1 (sampler) do not collide; keep them as written, or align both to the
  same scheme — just be consistent across shader and `ResourceBinding`.
- **Push-constant buffer index vs sampler/texture indices (Metal):** push
  constants use `buffer(0)`; texture/sampler use the texture/sampler namespaces,
  so there is no collision with `PushConstantBufferIndex = 0`. Confirm.
- **UV orientation:** Vulkan and Metal differ in clip/UV Y conventions; if the
  texture appears vertically flipped on one backend, flip the V coordinate in that
  backend's vertex shader (example-only change).
- **Static set lifetime:** `textureSet` references `texture` and `sampler`; all
  three must outlive the render loop and be destroyed before the device.

## What NOT to do in this step
- Do not add mipmaps, multiple textures, or a material system.
- Do not modify the gradient/compute path from Step 09.
- Do not add `CombinedImageSampler` or engine-library shaders.
