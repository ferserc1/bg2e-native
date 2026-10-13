# Tutorial 06: Cubemap Render Pass, Irradiance, and Specular Prefiltering

This tutorial adapts [GPU Cubemap Render Pass](../gpu/10_cubemap_render_pass.md)
to `bg2e::draw`. It uses `gpu::CubemapRenderPass` to generate an environment
cubemap, a diffuse irradiance map, and a specular reflection map with a
roughness-dependent mip chain. A rotating cube displays the selected map.

**Source:** [examples/draw/06_cubemap_render_pass/src/main.cpp](../../../examples/draw/06_cubemap_render_pass/src/main.cpp)

## What you will learn

- How to replace manual face iteration with `gpu::CubemapRenderPass`
- How to chain HDR conversion, diffuse convolution, and specular prefiltering
- How to render every face of every mip level with per-mip parameters
- Why each roughness level needs a separate uniform buffer during generation
- How to select a cubemap and sample an explicit mip level
- How to combine initialization-only GPU work with draw's managed frame lifecycle
- How to match fragment push constant memory layouts across Vulkan and Metal

## Prerequisites

- Completed [05_cubemap](05_cubemap.md)
- Familiarity with [03_uniform_buffers](03_uniform_buffers.md) and resource rings
- A configured bg2e-native build environment with a valid Vulkan SDK
- Compiled and packaged GLSL shaders and, on macOS, Metal shaders
- The Metal compiler toolchain on macOS
- The bundled [autumn_field_4k.hdr](../../../assets/autumn_field_4k.hdr) asset

Image loading uses `bg2e::db` in application code. Rendering resources and
commands use `bg2e::gpu`, with lifecycle and presentation managed by
`bg2e::draw`. The production `bg2e::render` path remains separate.

## Understanding the example

The expensive generation work runs once in `initScene()`, before the active
scene starts. All three outputs use `R16G16B16A16_SFLOAT`:

| Output | Face size | Mip levels | Purpose |
|--------|-----------|------------|---------|
| Original environment | 1024x1024 | 1 | HDR environment converted to a cubemap |
| Diffuse irradiance | 32x32 | 1 | Cosine-weighted hemisphere convolution |
| Specular reflection | 1024x1024 down to 8x8 | 8 | GGX prefiltering with increasing roughness |

The helper runs six face passes for each single-mip output, and 48 passes for
the eight-mip specular output: 60 face passes in total. These are three helper
invocations recorded in one synchronous immediate submission.

Each active scene frame only rotates and draws the visible cube. The UI selects
which map it samples; the specular map also accepts an explicit mip level.
The irradiance selection is a visualization through the same reflection shader,
not a complete diffuse lighting or PBR material implementation. The background
is a solid clear color, with no fullscreen skybox or tone-mapping pass.

Compared with tutorial 05, the fixed environment no longer needs conversion on
every scene update. Resize changes the visible cube's camera, depth image,
graphics pipeline, and model resource rings; generated maps keep their sizes.

## Step-by-step code explanation

### 1. Reuse the draw application lifecycle

The application installs scene, UI, and input delegates through
`app::Application`, then calls the `app::MainLoop::run()` overload taking
`draw::EngineConfig`. The default backend is Metal on macOS and Vulkan elsewhere.
The optional `--backend=vulkan` or `--backend=metal` argument overrides it.

The shared controller adds three values:

```cpp
uint32_t activeMap = 0;
uint32_t specularLod = 0;
uint32_t specularMipLevels = 1;
```

The map selector indexes three persistent resource sets in original, irradiance,
and specular order. Initialization replaces the default mip count with the
actual count returned by the specular cubemap.

### 2. Load shaders and create layouts

`gpu::ShaderLib` loads four shader pairs from `shaders/draw_cubemap_render_pass`:

| Shader pair | Vertex data | Fragment resources |
|-------------|-------------|--------------------|
| `equirect_to_cube` | Position and UV; projection/view push constants | HDR 2D texture and sampler |
| `irradiance_convolution` | Local cube direction; projection/view push constants | Original cubemap and sampler |
| `specular_prefilter` | Local cube direction; projection/view push constants | Original cubemap, sampler, and roughness UBO |
| `reflect_cube` | Position and normal; camera/model UBOs | Selected cubemap, sampler, camera position and LOD push constants |

The first three layouts have a 128-byte vertex push constant block containing
projection and view matrices. Their set 0 contains the source image and sampler.
The specular layout adds set 1 for roughness:

```cpp
specularLayoutDesc.resourceBindings.push_back(
    {1, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer,
     gpu::ShaderStage::Fragment, 1});
```

The visible cube uses the same camera/model arrangement as preceding examples:
set 0 is the camera, set 1 the model, and set 2 the selected cubemap and sampler.
Camera/model UBOs use Metal vertex buffer indices 2/3; roughness uses fragment
buffer index 1. Reflection push constants use fragment buffer index 0 on Metal.

### 3. Upload the HDR source and allocate cubemaps

The source image is loaded with `db::loadImage()` and uploaded into a sampled
RGBA16 floating-point image with `TransferDst` usage. An immediate submission
transitions it to `ShaderReadOnly` before generation.

The sampler enables linear mip filtering. The original and irradiance maps have
one mip; the specular map uses `minMipSize` to determine its mip count:

```cpp
auto specularCubeMap = device->createCubeMap({
    .size = originalSize,
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment |
             gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
    .minMipSize = 8,
    .debugName = "Specular reflection cubemap"
});
const uint32_t specularMipLevels = specularCubeMap->mipLevels();
```

The sizes are 1024, 512, 256, 128, 64, 32, 16, and 8. These are rendered levels,
not automatically generated downsampled copies: every level receives its own
roughness-dependent convolution.

### 4. Give each specular mip its own roughness buffer

The fragment UBO is padded to 16 bytes on both backends:

```cpp
struct RoughnessUBO {
    float roughness = 0.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};
```

Generation creates one buffer and one matching set for each mip. The important
part of the loop is:

```cpp
RoughnessUBO data{};
data.roughness = specularMipLevels > 1
                    ? float(mip) / float(specularMipLevels - 1)
                    : 0.0f;
auto ubo = device->createBuffer("Roughness UBO mip[" + std::to_string(mip) + "]");
_cleanup->push(ubo);
ubo->createUniformBuffer(data);
auto set = device->createResourceSet(specularLayout.get(), 1,
                                     "Roughness set mip[" + std::to_string(mip) + "]");
_cleanup->push(set);
set->setUniformBuffer({.vulkan = 0, .metal = 1}, ubo);
set->update();
roughnessSets.push_back(set);
```

All generation commands execute after recording. Repeatedly overwriting a
single UBO inside the recording callback would not preserve each mip's value.
Distinct immutable buffers let every recorded draw read the intended roughness.
These per-mip resources are separate from the per-frame model rings used later.

### 5. Build conversion meshes and pipelines

The equirectangular conversion draws an inverted sphere with position and UV
data (`gpu::MeshPU`). Irradiance and specular passes draw a direction cube with
the same vertex format; its local position supplies a sampling direction.
The visible reflective cube uses position, normal, and UV data (`gpu::MeshPNU`).

Conversion pipelines target `R16G16B16A16_SFLOAT`, use no depth attachment, and
disable culling. The reflection pipeline targets the surface color format and
uses an owned depth attachment created during `resize()`.

### 6. Chain three CubemapRenderPass invocations

`gpu::CubemapRenderPassInfo` enables clearing and automatic image transitions.
The helper provides a 90-degree, square projection and the view for each face,
opens/closes rendering, and iterates over every mip. The backend derives the
viewport/scissor from the current mip size.

The callback binds content and issues draw commands; it does not begin or end
another rendering pass. The specular invocation is:

```cpp
gpu::CubemapRenderPass::render(cmd, specularCubeMap.get(), genInfo,
    [&](gpu::CommandBuffer* c, gpu::CubemapFace, uint32_t mipLevel,
        const glm::mat4& projection, const glm::mat4& view)
    {
        CubemapCameraPushConstants pc{projection, view};
        c->bindPipeline(specularPipeline.get());
        c->bindResourceSet(specularPipeline.get(), 0, specularSourceSet.get());
        c->bindResourceSet(specularPipeline.get(), 1, roughnessSets[mipLevel].get());
        c->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(pc), &pc);
        dirCube.draw(c);
    });
```

The original conversion callback binds the HDR source and draws the sphere.
The irradiance callback binds the original cubemap and draws the direction cube.
All three calls are recorded, in that order, inside one `device->immediateSubmit()`.
Each output is transitioned to `ColorAttachment` before its full face/mip
iteration, then to `ShaderReadOnly` afterward. The original map is ready to
sample before the two convolution passes begin.

`immediateSubmit()` waits for this work to complete. Conversion-only mesh
buffers are then explicitly cleaned; persistent images, sets, buffers, layouts,
shader modules, and pipelines remain tracked by a local `gpu::CleanupManager`.

### 7. Update and render the visible cube

A persistent camera UBO changes only on resize, after draw drains GPU work.
Model Buffer/ResourceSet rings are rebuilt for the surface's frame count.
`update()` writes the model matrix using `context.frameSlot`; `render()` binds
the matching resource set after draw has waited for that slot to be reusable.

The visible pass transitions the owned depth image, opens rendering against
`context.colorTarget` and depth, and clears both attachments. Its core commands
are:

```cpp
ReflectPushConstants push{};
push.cameraPos = _cameraEye;
push.lod = _controller->activeMap == 2
               ? static_cast<float>(_controller->specularLod)
               : 0.0f;
command.bindPipeline(_pipeline.get());
command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
command.bindResourceSet(_pipeline.get(), 2, _cubeSets[_controller->activeMap].get());
_cube.draw(&command);
```

Original and irradiance maps always use LOD 0. Only the specular selection uses
the user-selected mip. Draw handles submission, retained scene composition,
UI composition, and presentation after the delegate finishes its pass.

### 8. Keep UI and input responsive while paused

The information window contains Reflection map, Next specular mip, Background,
Pause scene, Refresh scene, and scene/UI counters. The mip button is enabled
when the specular map is selected. The input delegate cycles maps on Space
release and mips on Enter release; application input receives keyboard events
when the UI does not capture them.

While active, UI drawing requests new scene frames to animate the cube. Pause
freezes both rotation and the retained scene while UI presentation continues.
Selection/background changes and refresh requests stay pending until resume.
Resizing while paused follows the clear-color behavior introduced in tutorial 01.

### 9. Clean resources after GPU completion

Draw drains outstanding work before resize and shutdown. Resize releases the
visible pipeline/depth and both model rings, updates the camera, and recreates
the window-sized resources. It does not regenerate the three cubemaps.

Shutdown cleans the visible pipeline, depth, model rings, and cube mesh, then
flushes the local CleanupManager in reverse creation order. Resource sets are
released before the images, buffers, and layouts they reference.

## Shader code explanation

### Environment conversion

The sphere's UVs sample the equirectangular HDR image. The helper-supplied
projection/view matrices orient the camera toward each cubemap face. This
preserves the source GPU example's conversion shaders and helper convention.

### Diffuse irradiance

The fragment shader builds a tangent basis around the normalized face direction
and integrates the environment over its hemisphere using angular steps of 0.05.
Each sample contributes `cos(theta) * sin(theta)`; the result is scaled by PI
and divided by the sample count. The small 32x32 output captures broad lighting
variation without retaining sharp environment details.

### Specular prefiltering

The fragment shader uses 64 Hammersley samples and GGX importance sampling.
For each sample it reflects the view direction about the sampled half-vector,
accumulates the original environment weighted by `max(dot(N, L), 0)`, then
normalizes by the accumulated weight. Roughness increases from 0 to 1 over the
eight rendered levels, progressively broadening the reflected environment.

### Reflection and explicit LOD

The GLSL fragment shader uses world-space position and normal:

```glsl
vec3 N = normalize(fragNormal);
vec3 I = normalize(fragWorldPos - push.cameraPos);
vec3 R = reflect(I, N);
outColor = textureLod(samplerCube(uCubeMap, uSampler), R, push.lod);
```

The matching Metal shader calls `sample(..., level(push.lod))`. Its camera
position uses `packed_float3`, followed by a float LOD, so the block matches the
host's 16-byte `glm::vec3` plus float layout. A regular Metal `float3` has a
larger alignment and would place LOD at a different offset. The application
asserts the host size and the LOD offset (12 bytes) at compile time.

All sixteen shaders follow the original GPU example except this explicit Metal
push constant packing adjustment and final newline normalization.

## Key concepts

- **Helper versus content:** CubemapRenderPass owns face/mip iteration, cameras,
  pass boundaries, and optional transitions; callbacks own pipelines and draws.
- **Rendered mip chain:** roughness convolution is performed separately for
  every level, rather than generated by a generic mipmap operation.
- **Immutable generation inputs:** per-mip buffers retain distinct roughness
  values across the recorded command buffer.
- **Managed active frames:** per-slot model rings support frames in flight;
  draw owns submission and presentation.
- **Retained output:** pausing suppresses scene work while keeping the UI active.

## Complete source

See [main.cpp](../../../examples/draw/06_cubemap_render_pass/src/main.cpp), the
[shader directory](../../../examples/draw/06_cubemap_render_pass/shaders/), and
[the helper header](../../../lib/include/bg2e/gpu/CubemapRenderPass.hpp).

## Building and running

The [CMakeLists.txt](../../../examples/draw/06_cubemap_render_pass/CMakeLists.txt)
uses the source GPU example's GLSL/Metal compilation and packaging calls with
the target renamed to `draw_cubemap_render_pass`. ShaderLib loads resources from
`shaders/draw_cubemap_render_pass`; the existing asset bundle supplies the HDR.

Use your configured platform build directory, replacing `build` as needed:

```sh
cmake --build build --target draw_cubemap_render_pass --config Debug
```

On macOS:

```sh
./bin/macos/draw_cubemap_render_pass.app/Contents/MacOS/draw_cubemap_render_pass --backend=metal
./bin/macos/draw_cubemap_render_pass.app/Contents/MacOS/draw_cubemap_render_pass --backend=vulkan
```

On Linux, run `./bin/linux/draw_cubemap_render_pass --backend=vulkan`.
On Windows, run `draw_cubemap_render_pass.exe` under `bin/windows`.
The macOS target compiles Metal shaders even when launching Vulkan.

To inspect the result:

1. Observe the rotating cube reflecting the original environment.
2. Select Diffuse irradiance to compare its broad color variation.
3. Select Specular reflection and cycle from mip 0 to 7 to inspect roughness.
4. Release Space/Enter with scene input active to exercise the same selections.
5. Pause, change a selection or background, then resume to apply it.
6. Resize while active and inspect projection, depth, and retained composition.

## Next steps

- [Ray Query Shadows](07_ray_query_shadows.md) — Combine rasterization with fragment acceleration structure queries.
- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Cubemap Render Pass](../gpu/10_cubemap_render_pass.md) — Compare the low-level implementation.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Reuse model resources safely across frame slots.
- [Draw API Reference](../../api/draw/index.md) — Retained composition and delegate lifecycle.
