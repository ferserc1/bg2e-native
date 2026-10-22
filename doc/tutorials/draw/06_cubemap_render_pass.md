# Tutorial 06: Cubemap Render Pass, Irradiance, and Specular Prefiltering

This tutorial adapts [GPU Cubemap Render Pass](../gpu/10_cubemap_render_pass.md)
to `bg2e::draw`. The current example uses public
`draw::environment::EnvironmentGenerator` to create shared sky, irradiance,
prefiltered specular and BRDF resources. A rotating cube displays a selected
cubemap while `SkyboxRenderer` draws the background. Tutorial GPU 10 retains
the lower-level `gpu::CubemapRenderPass` construction.

**Source:** [examples/draw/06_cubemap_render_pass/src/main.cpp](../../../examples/draw/06_cubemap_render_pass/src/main.cpp)

## What you will learn

- How to build and retain an immutable environment resource snapshot
- How to select sky, irradiance and prefiltered specular resources
- How to initialize the skybox against current surface formats
- How to combine synchronous generation with draw's managed frame lifecycle
- How to match reflection bindings and push constants across Vulkan and Metal

## Prerequisites

- Completed [05_cubemap](05_cubemap.md)
- Familiarity with [03_uniform_buffers](03_uniform_buffers.md) and resource rings
- A configured bg2e-native build environment with a valid Vulkan SDK
- Compiled and packaged GLSL shaders and, on macOS, Metal shaders
- The Metal compiler toolchain on macOS
- The bundled [autumn_field_4k.hdr](../../../assets/autumn_field_4k.hdr) asset

Image loading uses `bg2e::db` in application code. Rendering resources and
commands use `bg2e::gpu`, with lifecycle and presentation managed by
`bg2e::draw`. Draw is the production rendering API over GPU backends.

## Understanding the example

Generation runs synchronously once in `initScene()`. Default generation tools use
256-square sky/specular faces with complete mip chains, 32-square single-mip
irradiance and a 128-square BRDF integration image. The snapshot reports actual
formats and mip counts; do not hardcode the old sample's eight-level, 1024-to-8
specular chain. HDR radiance can require RGBA32F instead of the default RGBA16F.

Scene frames rotate the cube, draw the skybox, then sample the selected cubemap.
Irradiance selection visualizes that map through the reflection shader; it is
not a complete diffuse PBR implementation. Resize updates the visible camera,
depth, pipeline, model rings and skybox configuration, retaining generated maps.
See [environment ownership and HDR precision](../../api/draw/Environment.md).

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
actual count returned by prefilteredSpecular->mipLevels().

### 2. Load the visible reflection shaders and layout

`gpu::ShaderLib` loads `reflect_cube` vertex/fragment stages from the deployed
`draw_cubemap_render_pass` library. Its camera/model UBOs use Vulkan sets 0/1,
binding 0, and Metal vertex buffers 2/3. The selected map and sampler use Vulkan
set 2 bindings 0/1, Metal texture/sampler 0. Reflection push constants use Metal
fragment buffer 0. The paired shader files remain beside the example.

### 3. Generate and retain the environment

The linked source stores the generator, its shared immutable resource snapshot,
and a skybox member. In `initScene()`, with a live stopped Engine and shaderRoot:

```cpp
_environmentGenerator = std::make_unique<draw::environment::EnvironmentGenerator>(*_engine, shaderRoot);
_environmentResources = _environmentGenerator->build(base::PlatformTools::assetPath() / "autumn_field_4k.hdr");
_controller->specularMipLevels = _environmentResources->prefilteredSpecular->mipLevels();
_skybox.settings().shaderRoot = shaderRoot;
_skybox.setEnvironment(_environmentResources);
```

`build()` handles source decoding/upload and public conversion/IBL tools. It is
synchronous; it is not called on every scene frame. Retain Engine/Device until
the generator, snapshot, skybox and native resource consumers have been released.
The snapshot includes `sky`, `irradiance`, `prefilteredSpecular`, `brdf` and sampler.

### 4. Bind each snapshot image

Create persistent resource sets for the visible reflection layout:

```cpp
auto makeCubeSet = [this, device](const std::shared_ptr<gpu::Image>& map, const std::string& name)
{
    auto set = device->createResourceSet(_reflectLayout.get(), 2, name);
    _cleanup->push(set);
    set->setSampledImage({.vulkan = 0, .metal = 0}, map.get());
    set->setSampler({.vulkan = 1, .metal = 0}, _environmentResources->sampler.get());
    set->update();
    return set;
};
_cubeSets = {
    makeCubeSet(_environmentResources->sky, "Reflect original cube set"),
    makeCubeSet(_environmentResources->irradiance, "Reflect irradiance cube set"),
    makeCubeSet(_environmentResources->prefilteredSpecular, "Reflect specular cube set")
};
```

These raw image/sampler bindings borrow resources; the retained snapshot supplies
their lifetime. Binding a resource set does not transfer scene or Device ownership.

### 5. Configure visible geometry and skybox after resize

The cube uses `gpu::MeshPNU`, a camera UBO, per-slot model buffers/sets and an
owned depth image. Draw drains outstanding users before resize. Build the visible
pipeline with current color/depth formats, then initialize the skybox:

```cpp
_skybox.initialize(*_engine, surface->colorFormat(), _depth->pixelFormat());
_skybox.setCamera(camera.view, camera.projection);
```

The skybox uses the same retained environment and camera as the cube. Its native
shader resources come from the deployed draw environment library.

### 6. Compare with low-level cubemap generation

[GPU tutorial 10](../gpu/10_cubemap_render_pass.md) still teaches direct face/mip
iteration, per-mip roughness UBOs, conversion meshes and explicit submissions.
Those are lower-level construction examples, not code to insert into this
example's current `initScene()`. EnvironmentGenerator owns its public generation
tools, including BRDF integration, and validates their formats/interfaces. Compatible
custom tools can be supplied through its configuration before building; see the
[public environment reference](../../api/draw/Environment.md).

### 7. Update and render the visible cube

A persistent camera UBO changes only on resize, after draw drains GPU work.
Model Buffer/ResourceSet rings are rebuilt for the surface's frame count.
`update()` writes the model matrix using `context.frameSlot`; `render()` binds
the matching resource set after draw has waited for that slot to be reusable.

The visible pass transitions the owned depth image, opens rendering against
`context.colorTarget` and depth, and clears both attachments, then calls `_skybox.draw(command, context.extent)`
before the cube. Its reflection commands
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
released before the images, buffers, and layouts they reference. Then cleanup
releases skybox registrations, the shared environment snapshot and the generator
while Engine/Device remain alive.

## Shader code explanation

### Environment generation shaders

The current application delegates conversion, irradiance, specular convolution
and BRDF integration to paired native libraries under shaders/src/draw/environment.
Their interface/version and precision rules belong to the public environment API;
the old sample-local conversion shaders are not this example's execution path.
Use [GPU tutorial 10](../gpu/10_cubemap_render_pass.md) for the manual helper recipe.

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

- **Immutable snapshots:** retain generated images/sampler through all consumers.
- **Explicit native bindings:** Vulkan set/binding and Metal indices differ.
- **Completed slots:** model writes follow acquisition/retirement, using frameSlot.
- **Stopped lifecycle:** generation, resize and cleanup run with producers stopped.
- **Actual mip counts:** cycle the runtime count rather than a historical constant.
- **No runtime claim:** these snippets are source-checked, not executed validation.

## Building and running

The [CMakeLists.txt](../../../examples/draw/06_cubemap_render_pass/CMakeLists.txt)
registers `draw_cubemap_render_pass` and its paired visible shaders. ShaderLib
loads reflection stages from `shaders/draw_cubemap_render_pass`; the engine's
paired draw environment libraries provide generation/skybox stages, and the
existing asset bundle supplies the HDR.

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
3. Select Specular reflection and cycle through the reported mip count to inspect roughness.
4. Release Space/Enter with scene input active to exercise the same selections.
5. Pause, change a selection or background, then resume to apply it.
6. Resize while active and inspect projection, depth, and retained composition.

## Next steps

- [Ray Query Shadows](07_ray_query_shadows.md) — Combine rasterization with fragment acceleration structure queries.
- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Cubemap Render Pass](../gpu/10_cubemap_render_pass.md) — Compare the low-level implementation.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Reuse model resources safely across frame slots.
- [Draw API Reference](../../api/draw/index.md) — Retained composition and delegate lifecycle.
