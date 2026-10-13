# Tutorial 05: Cubemap Rendering and Mirror Reflection

This tutorial adapts [GPU Cubemap](../gpu/09_cubemap.md) to `bg2e::draw`.
It loads an equirectangular HDR environment, renders it into six cubemap faces,
and displays a rotating mirror cube using per-pixel environment reflection.
The information window includes a direct cubemap debug mode alongside the
background, pause, and refresh controls from earlier draw examples.

**Source:** [examples/draw/05_cubemap/src/main.cpp](../../../examples/draw/05_cubemap/src/main.cpp)

## What you will learn

- How to load and upload a floating-point HDR image
- How to create a sampled cubemap that can also be a render target
- How to render a mesh into individual cubemap faces
- How to provide projection/view matrices through vertex push constants
- How to sample a cubemap using reflected view directions
- How to update model and rendering settings through frame-slot resource rings
- How to combine cubemap rendering with retained scene/UI composition

## Prerequisites

- Completed [03_uniform_buffers](03_uniform_buffers.md)
- Familiarity with multiple passes from [04_render_to_texture](04_render_to_texture.md)
- A configured bg2e-native build environment with the required Vulkan SDK
- The GLSL and, on macOS, Metal shaders compiled and packaged by the target
- On macOS, the Metal compiler toolchain
- The bundled [autumn_field_4k.hdr](../../../assets/autumn_field_4k.hdr) environment asset

The example uses CPU image loading through `bg2e::db::loadImage()` in application
code and creates all rendering resources through `bg2e::gpu`. It does not use
or change the production `bg2e::render` path.

## Understanding the example

The scene performs two groups of rendering passes on each active update:

1. **Environment conversion:** draw an inward-facing textured sphere into each
   of the six 512x512 floating-point cubemap faces using a 90-degree camera.
2. **Mirror cube:** draw a rotating cube with position and normal data into the
   retained scene target, using a delegate-owned depth image and the cubemap.

The default fragment shader mode reflects the camera-to-surface incident vector
around the surface normal and samples the environment in that direction.
Direct cubemap debug mode instead samples using the normalized local cube
position, making the cubemap content easier to inspect.

The background is a solid clear color. The environment is reflected by the
cube; it is not drawn as a fullscreen skybox. Pause freezes the retained result
and rotation while the UI remains active.

## Step-by-step code explanation

### 1. Reuse the draw application and shared controller

The application follows the same Application/MainLoop entry point as previous
examples and selects the experimental draw path with `draw::EngineConfig`.
The default backend is Metal on macOS and Vulkan elsewhere; the optional
`--backend=vulkan` or `--backend=metal` argument overrides it.

`Controller` adds `debugCubemapMode`, initially false. The UI exposes this value
with a Direct cubemap debug checkbox and invalidates the scene when it changes.
The input delegate also toggles it when Space is released, provided the UI is
not capturing keyboard input. Releasing rather than pressing prevents keyboard
auto-repeat from repeatedly changing modes.

The UI continues requesting scene frames while active, as in tutorial 03.
Changing modes, Background, or Refresh scene while paused leaves a pending
change until resume.

### 2. Match the shader data structures

```cpp
struct CubemapCameraPushConstants {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
};

struct CubeReflectionPushConstants {
    glm::vec3 cameraPos;
    float padding = 0.0f;
};

struct ModelUBO {
    glm::mat4 model{1.0f};
};

struct CameraUBO {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 cameraPos{0.0f};
};

struct CubeRenderSettingsUBO {
    uint32_t mode = 0; // 0 = reflection, 1 = direct cubemap debug
    uint32_t _pad0 = 0;
    uint32_t _pad1 = 0;
    uint32_t _pad2 = 0;
};
```

The cubemap face camera uses two matrices in vertex push constants: projection
and view. The mirror cube uses a persistent camera UBO, a per-slot model UBO,
and fragment push constants containing the camera position. Padding keeps the
fragment push data at 16 bytes.

The settings UBO has a mode and three padding values, matching both shader
languages. Mode 0 selects mirror reflection; mode 1 selects direct sampling.
Model matrices are initialized to identity, including the sphere model.

**Reference:** [PipelineLayout](../../api/gpu/PipelineLayout.md), [Buffer](../../api/gpu/Buffer.md)

### 3. Load the two shader pairs

`gpu::ShaderLib` loads `cubemap_renderer` and `reflect_cube` from
`PlatformTools::shaderPath() / "draw_cubemap"`. Both shader pairs have GLSL and
Metal equivalents. The current loader uses `main` for Vulkan modules and
`vertMain`/`fragMain` for Metal modules.

The four modules, persistent layouts, HDR image, sampler, cubemap, camera
resources, and cubemap renderer pipeline are registered with a local
`CleanupManager`. The manager borrows the engine surface but only owns the
resources pushed by this delegate.

### 4. Define separate layouts for conversion and reflection

The cubemap renderer layout declares a 128-byte vertex push constant range and
these bindings:

| Set | Vulkan binding | Metal argument | Resource | Stage |
|-----|----------------|----------------|----------|-------|
| 0 | 0 | `buffer(2)` | Sphere model UBO | Vertex |
| 1 | 0 | `texture(0)` | Equirectangular HDR image | Fragment |
| 1 | 1 | `sampler(0)` | Sampler | Fragment |

Metal vertex push constants use `buffer(1)`; Vulkan uses the declared byte
offset 0. The fragment sampler index is 0 in this example, unlike the texture
sampler in the preceding cube examples.

The reflection layout has a 16-byte fragment push constant range and four sets:

| Set | Vulkan binding | Metal argument | Resource | Stage |
|-----|----------------|----------------|----------|-------|
| 0 | 0 | `buffer(2)` | Camera UBO | Vertex |
| 1 | 0 | `buffer(3)` | Cube model UBO | Vertex |
| 2 | 0 | `texture(0)` | Sampled cubemap | Fragment |
| 2 | 1 | `sampler(0)` | Sampler | Fragment |
| 3 | 0 | `buffer(3)` | Render settings UBO | Fragment |

Metal fragment push constants use `buffer(0)`. The model and settings both use
buffer index 3, but are bound to different shader stages.

**Reference:** [PipelineLayout](../../api/gpu/PipelineLayout.md), [ResourceSet](../../api/gpu/ResourceSet.md)

### 5. Load the HDR environment

```cpp
// Equirectangular HDR texture
auto assetsPath = base::PlatformTools::assetPath();
auto imageData = std::shared_ptr<base::Image>(db::loadImage(assetsPath / "autumn_field_4k.hdr"));
if (!imageData || imageData->width() == 0 || imageData->height() == 0)
{
    throw std::runtime_error("Failed to load the cubemap HDR environment.");
}
_equirectTexture = device->createImage({.size = {imageData->width(), imageData->height()},
                                            .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                            .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                            .debugName = "Equirect HDR texture"});
_cleanup->push(_equirectTexture);
_equirectTexture->uploadImage(imageData.get());

device->immediateSubmit([this](gpu::CommandBuffer* cmd)
                        {
                            cmd->transition(_equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
                        });
_sampler = device->createSampler({.debugName = "Default linear sampler"});
_cleanup->push(_sampler);
```

The CPU loader reads the bundled HDR asset. A descriptive error is raised if
it returns no image or zero dimensions. The GPU image uses RGBA16 floating-point
format, is uploaded through `uploadImage()`, and transitions to `ShaderReadOnly`
before use.

The equirectangular resource set belongs to the conversion layout's set 1 and
binds the HDR image with the sampler. Its contents remain unchanged across
frames and resizes.

**Reference:** [Image](../../api/gpu/Image.md)

### 6. Create the renderable cubemap and sampled resource set

```cpp
// Cubemap render target
const uint32_t cubemapSize = 512;
_cubeMap = device->createCubeMap({.size = cubemapSize,
                                      .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
                                      .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst,
                                      .mipLevels = 1,
                                      .debugName = "Skybox cubemap"});
_cleanup->push(_cubeMap);

// Cubemap texture resource set (set 2): cubemap image + sampler, persistent.
_cubemapTextureSet = device->createResourceSet(_cubeReflectionLayout.get(), 2, "Cubemap texture set");
_cubemapTextureSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, _cubeMap.get());
_cubemapTextureSet->setSampler({.vulkan = 1, .metal = 0}, _sampler.get());
_cubemapTextureSet->update();
_cleanup->push(_cubemapTextureSet);
```

The cubemap has six square 512x512 faces, one mip level, and the same floating-
point format as the HDR texture. Color attachment usage permits face rendering;
sampled usage permits reflection lookup. The transfer flags preserve the
original GPU example's description, although this example does not use them.

`setSampledCubeMap()` binds a cubemap view rather than a regular 2D image view.
The resulting set is persistent because the cubemap's size does not follow
window resizing.

**Reference:** [CubeMap header](../../../lib/include/bg2e/gpu/CubeMap.hpp), [ResourceSet](../../api/gpu/ResourceSet.md)

### 7. Create the sphere, cube, and camera resources

The conversion mesh is `geo::createSpherePU(50.0f, 16, 16, true)`. It has position
and UV coordinates and inward-facing triangles. The reflection mesh is
`geo::createCubePNU(1.0f, 1.0f, 1.0f)`, with position, normal, and UV attributes.
Each CPU mesh is owned temporarily by `unique_ptr`, copied into its GPU mesh,
and uploaded during scene initialization.

The conversion pipeline uses `MeshPU::vertexBufferDescription()`, the floating-
point cubemap format, no depth attachment, and no culling. The reflection
pipeline uses `MeshPNU::vertexBufferDescription()`, the retained target's color
format, and the delegate-owned depth image's format.

The persistent camera UBO is initialized once, then updated during synchronized
resize with a 60-degree perspective projection, the actual target aspect ratio,
and a camera at `(0, 0, 3)` looking at the origin.

**Reference:** [Mesh](../../api/gpu/Mesh.md), [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 8. Build three pairs of resource rings

Resize creates one buffer/set pair per in-flight slot for each of these uses:

| Ring pair | Data | Pipeline set |
|-----------|------|--------------|
| Sphere model | Identity model matrix | Cubemap renderer set 0 |
| Cube model | Animated rotation matrix | Reflection set 1 |
| Cube settings | Reflection/debug mode | Reflection set 3 |

Every resource set binds the buffer from the matching ring slot through
`sharedAt(slot)`. The update and render callbacks select entries explicitly
with `context.frameSlot`. Descriptor contents remain unchanged between resizes.

The acquired frame slot has been made reusable before draw invokes `update()`:

```cpp
void update(const draw::FrameContext& context) override
{
    _time += context.deltaSeconds;
    _cubemapModelUboRing.at(context.frameSlot)->updateUniformBuffer(ModelUBO{});
    ModelUBO model{};
    model.model = glm::rotate(glm::mat4(1.0f), _time,
                              glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f)));
    _cubeModelUboRing.at(context.frameSlot)->updateUniformBuffer(model);
    CubeRenderSettingsUBO settings{};
    settings.mode = _controller->debugCubemapMode ? 1u : 0u;
    _cubeSettingsUboRing.at(context.frameSlot)->updateUniformBuffer(settings);
}
```

The cube rotates around the normalized axis `(4, 1, 0.5)`, preserving the source
example. Time advances only for active scene updates. Mode settings are updated
in the same slot as the model matrix, so in-flight readers keep their own data.

**Reference:** [FrameResourceRing](../../api/gpu/FrameResourceRing.md), [FrameContext](../../api/draw/FrameContext.md)

### 9. Keep the original cubemap face camera convention

All face cameras sit at the origin and use a 90-degree projection with aspect
ratio 1. The camera table is copied from the source example:

```cpp
const std::array<CubemapFaceCamera, 6> faceCameras = {{
    {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},  // +X
    {glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)}, // -X
    {glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, // +Y
    {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},   // -Y
    {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},  // +Z
    {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}, // -Z
}};
```

The array order matches `gpu::CubemapFace`: +X, -X, +Y, -Y, +Z, -Z. Preserve
the original center and up-vector convention, including the reversed Y-axis
camera directions shown in the table, when comparing the two examples.

### 10. Render the cubemap and mirror cube

```cpp
void render(const draw::FrameContext& context) override
{
    auto& command = context.commandBuffer;
    // Preserve the original example's six face passes on each active update.
    const auto projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& camera = faceCameras[face];
        CubemapCameraPushConstants push{};
        push.projection = projection;
        push.view = glm::lookAt(camera.eye, camera.center, camera.up);
        command.transition(_cubeMap->image(), gpu::ImageLayout::ColorAttachment);
        command.beginRendering(_cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
        command.clearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
        command.bindPipeline(_cubemapRendererPipeline.get());
        command.bindResourceSet(_cubemapRendererPipeline.get(), 0, _cubemapModelSetRing.at(context.frameSlot));
        command.bindResourceSet(_cubemapRendererPipeline.get(), 1, _equirectTextureSet.get());
        command.pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(push), &push);
        _cubemapSphere.draw(&command);
        command.endRendering();
    }
    command.transition(_cubeMap->image(), gpu::ImageLayout::ShaderReadOnly);

    command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
    command.beginRendering(&context.colorTarget, _depth.get());
    const auto& color = _controller->color;
    command.clearColor(0, {color.r, color.g, color.b, color.a});
    command.clearDepth(1.0f);
    CubeReflectionPushConstants push{};
    push.cameraPos = _cameraEye;
    command.bindPipeline(_cubeReflectionPipeline.get());
    command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(push), &push);
    command.bindResourceSet(_cubeReflectionPipeline.get(), 0, _cameraSet.get());
    command.bindResourceSet(_cubeReflectionPipeline.get(), 1, _cubeModelSetRing.at(context.frameSlot));
    command.bindResourceSet(_cubeReflectionPipeline.get(), 2, _cubemapTextureSet.get());
    command.bindResourceSet(_cubeReflectionPipeline.get(), 3, _cubeSettingsSetRing.at(context.frameSlot));
    _cube.draw(&command);
    command.endRendering();
    ++_controller->sceneFrames;
}
```

`beginRendering(CubeMap*, face, mipLevel)` selects a face attachment; each face
pass closes before the next starts. The sphere model comes from the current
slot, while the face projection and view are supplied through vertex push
constants. After all six faces, the cubemap image transitions to
`ShaderReadOnly` before reflection sampling.

The main pass uses the retained color target supplied by draw and the depth
image owned by the delegate. It clears Background and depth, binds camera,
model, cubemap, and settings sets, supplies the camera position as fragment push
constants, and draws the cube. The UI is composed later by draw.

The original GPU example regenerates all six faces every frame. This adaptation
preserves that demonstration on every active scene update, even though the
loaded HDR environment is static. Paused presentation does not invoke these
scene passes.

**Reference:** [CommandBuffer](../../api/gpu/CommandBuffer.md), [RenderLoop](../../api/draw/RenderLoop.md)

### 11. Resize and clean up after GPU completion

Draw waits for prior GPU work before calling `resize()`. The callback replaces
the reflection pipeline and depth image, cleans and clears all six resource
rings, updates the camera aspect ratio, and recreates the per-slot resources.
The 512x512 cubemap and conversion pipeline remain unchanged.

Shutdown follows the same synchronized lifecycle. The reflection pipeline,
depth image, and rings are cleaned explicitly; sets are cleaned before their
buffers. Both mesh containers release their vertex/index buffers, then the
local cleanup manager releases persistent resources in reverse push order.
Draw retains ownership of its engine, surface, and device throughout.

## Shader code explanation

### Cubemap renderer

`cubemap_renderer.vert.glsl` transforms sphere positions with the pushed face
projection/view matrices and the sphere model UBO. It forwards the sphere's UV
coordinates to the fragment shader:

```glsl
#version 450

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform sampler uSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

The sphere's UV mapping supplies equirectangular texture coordinates. Rendering
from its center in six directions projects the panorama onto cubemap faces;
no compute conversion is used here.

### Reflection vertex shader

`reflect_cube.vert.glsl` transforms local positions into world space and then
clip space. It forwards world position, model-transformed normal, and local
position. The model only rotates, so its 3x3 linear component can transform
normals directly for this example.

### Reflection fragment shader (`reflect_cube.frag.glsl`)

```glsl
#version 450

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler uSampler;

layout(set = 3, binding = 0) uniform CubeRenderSettings
{
    uint mode;
    uint _pad0;
    uint _pad1;
    uint _pad2;
}
settings;

layout(push_constant) uniform PushConstants
{
    vec3 cameraPos;
}
push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

void main()
{
    if (settings.mode == 0)
    {
        vec3 N = normalize(fragNormal);
        vec3 I = normalize(fragWorldPos - push.cameraPos);
        vec3 R = reflect(I, N);
        outColor = texture(samplerCube(uCubeMap, uSampler), R);
    }
    else
    {
        vec3 D = normalize(fragLocalPos);
        outColor = texture(samplerCube(uCubeMap, uSampler), D);
    }
}
```

In reflection mode, `I` points from the camera toward the surface. `reflect(I, N)`
produces the environment lookup direction. The cubemap sampler selects the
appropriate face from that direction.

In debug mode, normalized local position selects the lookup direction instead.
The cubemap pattern is anchored to the cube's local geometry, making this a
useful comparison against world-space reflection while the cube rotates.

### Metal equivalents

The Metal shaders use the same algorithms and mappings listed in the binding
tables. The reflection fragment shader samples `texturecube<float>` with
`sampler(0)`, reads its settings at `buffer(3)`, and receives camera position
through fragment push constants at `buffer(0)`.

## Key concepts

### Face rendering and whole-image transitions

Face rendering chooses a layer of the cubemap, while `cubeMap->image()` exposes
the underlying image for transitions. The delegate transitions to
`ColorAttachment` for the face passes and to `ShaderReadOnly` after them.
Transitions order writes before sampling, including across consecutive active
frames using the same graphics queue.

### Fixed environment resources and resize-dependent scene resources

| Resources | Lifecycle |
|-----------|-----------|
| HDR texture, sampler, cubemap, cubemap sampling sets | Persistent |
| Conversion shaders, layout, and graphics pipeline | Persistent |
| Sphere and cube GPU meshes | Persistent |
| Camera UBO/set | Persistent; contents updated on resize |
| Model/settings rings | Recreated on synchronized resize |
| Reflection graphics pipeline and scene depth image | Recreated on synchronized resize |

### Reflection/debug controls and retained pause

The debug checkbox and Space shortcut update the same controller value. Input
routing respects the UI's keyboard capture; the checkbox remains available when
keyboard events belong to the UI. Mode changes while paused stay pending.

Resize while paused replaces the retained image using draw's stored pause clear
color. The next active scene frame renders the cubemap and cube at the new scene
extent. The fixed environment resolution stays 512 pixels per face.

## Complete render loop flow

```text
initScene:
  load HDR -> upload image -> create cubemap and resource layouts
  build sphere/cube meshes -> create conversion pipeline and camera resources
first drawable or resize:
  drain prior GPU work (draw)
  update camera projection -> create depth/reflection pipeline -> rebuild rings
active scene frame:
  update current slot's sphere model, cube model, and settings
  render sphere into six cubemap faces
  transition cubemap to ShaderReadOnly
  draw mirror/debug cube into retained color + owned depth
presentation:
  copy retained scene -> compose UI -> submit -> present (draw)
shutdown:
  drain GPU work -> clean pipeline/depth/rings/meshes -> flush persistent resources
```

## Complete source

See [main.cpp](../../../examples/draw/05_cubemap/src/main.cpp) and the
[shader directory](../../../examples/draw/05_cubemap/shaders/).
The eight shader files preserve the original GPU example's code.

## Building and running

The [CMakeLists.txt](../../../examples/draw/05_cubemap/CMakeLists.txt) uses the
source GPU example's explicit GLSL/Metal compilation and resource packaging
calls, with the target renamed to `draw_cubemap`. Shader modules are packaged
under `shaders/draw_cubemap`; the existing asset bundle supplies the HDR file.

Use your configured platform build directory, replacing `build` as needed:

```sh
cmake --build build --target draw_cubemap --config Debug
```

On macOS:

```sh
./bin/macos/draw_cubemap.app/Contents/MacOS/draw_cubemap --backend=metal
./bin/macos/draw_cubemap.app/Contents/MacOS/draw_cubemap --backend=vulkan
```

On Linux, run `./bin/linux/draw_cubemap --backend=vulkan`.
On Windows, run `draw_cubemap.exe` under `bin/windows`.
The macOS target includes Metal shader compilation even when launching Vulkan.

To inspect the result:

1. Observe the environment reflection on the rotating cube.
2. Toggle Direct cubemap debug to compare local-direction sampling.
3. Release Space with scene input active to toggle the same mode.
4. Change Background and pause; UI counters continue while the scene stays frozen.
5. Change mode while paused, then resume to apply it.
6. Resize while active to inspect the adjusted projection and depth attachment.

## Next steps

- [Cubemap Render Pass](06_cubemap_render_pass.md) — Generate irradiance and specular maps with a face/mip helper.
- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Cubemap](../gpu/09_cubemap.md) — Compare the lower-level implementation.
- [ResourceSet](../../api/gpu/ResourceSet.md) — Cube texture bindings and stage-specific resources.
- [Draw API Reference](../../api/draw/index.md) — Retained composition and delegate lifecycle.
