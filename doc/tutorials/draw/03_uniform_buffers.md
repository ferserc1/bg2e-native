# Tutorial 03: Uniform Buffers and a Rotating Textured Cube

This tutorial adapts [GPU Uniform Buffers](../gpu/07_uniform_buffers.md) to
`bg2e::draw`. It renders a rotating textured cube using a persistent camera
uniform buffer, per-frame model buffers, and a depth attachment owned by the
scene delegate. The information window and pause controls follow
[02_simple_triangle](02_simple_triangle.md).

**Source:** [examples/draw/03_uniform_buffers/src/main.cpp](../../../examples/draw/03_uniform_buffers/src/main.cpp)

## What you will learn

- How to bind camera, model, and material resources in separate descriptor sets
- How to create and update uniform buffers with C++ matrix structures
- How to use `FrameResourceRing` with draw's in-flight frame slots
- How to supply a depth attachment alongside the retained color target
- How to update the projection and rebuild resources after resize
- How to use a local `CleanupManager` for persistent resources
- How to freeze animation while retaining scene/UI composition

## Prerequisites

- Completed [01_window_ui](01_window_ui.md) and [02_simple_triangle](02_simple_triangle.md)
- A configured bg2e-native build environment with the required Vulkan SDK
- The example's shaders compiled and packaged by its CMake target
- On macOS, the Metal compiler toolchain for the `.metal` shader sources
- Optional: [GPU Uniform Buffers](../gpu/07_uniform_buffers.md) for the original implementation

## Understanding the example

The cube uses a perspective camera at `(0, 0, 3)` looking at the origin.
A model matrix rotates it around the normalized axis `(0.4, 1.0, 0.2)`.
Its material samples the same red/green/blue/yellow 2x2 texture used in tutorial
02. This example uses a solid background and does not need a compute pipeline.

The scene delegate creates its own depth image because `FrameContext` supplies
only the retained color target. It renders directly into that color image using
`beginRendering(colorImage, depthImage)`. Draw continues to own acquisition,
command submission, retained-image copying, UI composition, and presentation.

The shader resources are split into three sets:

| Set | Resource | Update frequency |
|-----|----------|------------------|
| 0 | Camera projection/view uniform buffer | On resize, after GPU work is drained |
| 1 | Model transform uniform buffer | Each active scene update, for the acquired frame slot |
| 2 | Texture and sampler | Initialized once |

## Step-by-step code explanation

### 1. Reuse Application/MainLoop and the UI controls

The application uses the draw configuration overload of `MainLoop::run()`.
Backend selection accepts `--backend=vulkan` and `--backend=metal`; the defaults
remain Metal on macOS and Vulkan elsewhere. Metal is rejected on other platforms.

The UI provides Backend, UI frames, Scene updates, Background, Pause scene,
and Refresh scene. It requests scene frames while active so the cube rotates
continuously. Pausing retains the last image, stops scene updates, and keeps
the interface interactive. There is no ImGui demo window.

The Background picker now controls the solid clear color. Changes made while
paused remain pending until resume, including refresh requests.

### 2. Define the uniform data structures

```cpp
struct CameraUBO {
    glm::mat4 projectionView{1.0f};
};

struct ModelUBO {
    glm::mat4 model{1.0f};
};
```

Each structure holds a 4x4 matrix initialized to identity. These structures
match the `mat4` fields in GLSL and `float4x4` fields in Metal. Camera data
combines projection and view; model data transforms local cube positions.

```text
clip position = projectionView * model * local position
```

Unlike tutorial 02's small color push constants, these matrices live in uniform
buffers and are bound through resource sets.

**Reference:** [Buffer](../../api/gpu/Buffer.md), [PipelineLayout](../../api/gpu/PipelineLayout.md)

### 3. Load shaders and prepare local cleanup

```cpp
auto* device = _engine->device();
// Keep static DeviceResource teardown local to this delegate.
_cleanup = std::make_unique<gpu::CleanupManager>(_engine->surface());
gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_uniform_buffers",
                       _engine->backendType());
_vertexShader = shaders.vertex("cube", device);
_cleanup->push(_vertexShader);
_fragmentShader = shaders.fragment("cube", device);
_cleanup->push(_fragmentShader);
```

The local cleanup manager borrows the engine surface but only tracks this
delegate's persistent `DeviceResource` objects. It does not own the surface,
engine, or device. Resources are pushed after creation and released in reverse
push order during delegate cleanup.

ShaderLib loads the `cube` vertex and fragment modules from
`PlatformTools::shaderPath() / "draw_uniform_buffers"`. Vulkan uses `.spv`
files with the loader's `main` entry point; Metal uses `.metallib` files with
`vertMain` and `fragMain`.

**Reference:** [CleanupManager](../../api/gpu/CleanupManager.md), [ShaderModule](../../api/gpu/ShaderModule.md)

### 4. Describe the three resource sets

```cpp
gpu::PipelineLayoutDescription layout{};
layout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
layout.resourceBindings.push_back({1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1});
layout.resourceBindings.push_back({2, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
layout.resourceBindings.push_back({2, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
layout.debugName = "Cube pipeline layout";
_layout = device->createPipelineLayout(layout);
_cleanup->push(_layout);
```

The layout maps Vulkan set/binding pairs to explicit Metal resource indices:

| Set | Vulkan binding | Metal argument | Type | Stage |
|-----|----------------|----------------|------|-------|
| 0 | 0 | `buffer(2)` | Camera uniform buffer | Vertex |
| 1 | 0 | `buffer(3)` | Model uniform buffer | Vertex |
| 2 | 0 | `texture(0)` | Sampled image | Fragment |
| 2 | 1 | `sampler(1)` | Sampler | Fragment |

Vulkan permits binding 0 in multiple sets. Metal buffer arguments use distinct
indices for the camera and model. Texture and sampler indices occupy separate
Metal namespaces.

**Reference:** [PipelineLayout](../../api/gpu/PipelineLayout.md), [ResourceSet](../../api/gpu/ResourceSet.md)

### 5. Create the texture, sampler, and material set

During `initScene()`, the delegate creates an RGBA8 2x2 image with `Sampled` and
`TransferDst` usage, uploads the four texels, and transitions it to
`ShaderReadOnly` in a one-time submission. A default linear sampler is created
alongside it.

The material set belongs to set 2:

```cpp
_textureSet = device->createResourceSet(_layout.get(), 2, "Material resource set");
_cleanup->push(_textureSet);
_textureSet->setSampledImage({.vulkan = 0, .metal = 0}, _texture.get());
_textureSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
_textureSet->update();
```

These bindings remain unchanged across frames and resizes. See tutorial 02 for
the texture upload sequence.

### 6. Create a persistent camera UBO and the cube mesh

```cpp
// Camera contents change only during resize, after draw drains GPU work.
_cameraUbo = device->createBuffer("Camera UBO");
_cleanup->push(_cameraUbo);
_cameraUbo->createUniformBuffer(CameraUBO{});
_cameraSet = device->createResourceSet(_layout.get(), 0, "Camera resource set");
_cleanup->push(_cameraSet);
_cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, _cameraUbo);
_cameraSet->update();

std::unique_ptr<geo::MeshPU> cubeData(geo::createCubePU(1.0f, 1.0f, 1.0f));
_cube.setMeshData(*cubeData);
_cube.build(device);
```

`createUniformBuffer(CameraUBO{})` allocates camera storage. Its resource set
binds that buffer at set 0 using Metal index 2. The camera buffer and its set
persist throughout the scene lifecycle; resize changes only the buffer contents.

`geo::createCubePU()` returns a CPU mesh with position and UV data. The local
`unique_ptr` owns that temporary mesh. `_cube` copies it and builds persistent
GPU vertex and index buffers.

**Reference:** [Buffer](../../api/gpu/Buffer.md), [Mesh](../../api/gpu/Mesh.md)

### 7. Update the camera projection on resize

Draw calls `resize()` before the first scene render and whenever it replaces
the retained target. It waits for in-flight work before invoking the callback.
The delegate first releases its old graphics pipeline, depth image, and model
rings, then updates the camera using the actual drawable extent:

```cpp
const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
const auto projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
const auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                              glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));
CameraUBO cameraData{};
cameraData.projectionView = projection * view;
_cameraUbo->updateUniformBuffer(cameraData);
```

The perspective field of view is 60 degrees, with near/far planes at 0.1 and
100. The aspect ratio comes from the scene image rather than the initial window
size. This keeps the projection consistent with the target after resize.

Updating the shared camera buffer here is safe because draw has drained prior
readers. If camera movement is later added to every frame, its mutable data
should also use per-slot storage or another suitable synchronization strategy.

### 8. Create an owned depth image

```cpp
_depth = device->createImage({.size = extent,
                             .format = surface->depthFormat(),
                             .usage = gpu::ImageUsage::DepthStencil,
                             .debugName = "Cube depth attachment"});
```

The depth image matches the retained color target's dimensions and uses the
surface's depth format. It belongs to the delegate and is not a depth image
borrowed from an acquired presentation frame. The delegate clears it on every
scene render, so no depth content needs to be retained while paused.

**Reference:** [Image](../../api/gpu/Image.md), [FrameContext](../../api/draw/FrameContext.md)

### 9. Build per-frame model buffer and resource-set rings

```cpp
_modelUboRing.create(surface, [device](uint32_t slot)
{
    auto buffer = device->createBuffer("Model UBO slot[" + std::to_string(slot) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});
_modelSetRing.create(surface, [this, device](uint32_t slot)
{
    auto set = device->createResourceSet(_layout.get(), 1,
                                         "Model resource set slot[" + std::to_string(slot) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, _modelUboRing.sharedAt(slot));
    set->update();
    return set;
});
```

`FrameResourceRing::create()` allocates one entry per `surface->inFlightFrames()`.
Each model set in ring slot `i` points to the buffer in that same slot through
`sharedAt(i)`. Descriptor bindings are populated once during ring creation.

Resize recreates both rings after GPU completion, using the surface's current
in-flight slot count. Resource sets are cleaned before the buffers they refer
to; `clear()` then removes the ring's stored pointers and surface reference.

A frame slot is an in-flight submission slot, not the acquired swapchain image
index. The delegate uses `context.frameSlot` for both update and binding.

**Reference:** [FrameResourceRing](../../api/gpu/FrameResourceRing.md)

### 10. Create the depth-enabled graphics pipeline

```cpp
gpu::GraphicsPipelineDescription pipeline{};
pipeline.vertexShader = _vertexShader.get();
pipeline.fragmentShader = _fragmentShader.get();
pipeline.layout = _layout.get();
pipeline.colorFormat = surface->colorFormat();
pipeline.depthFormat = _depth->pixelFormat();
pipeline.debugName = "Textured cube pipeline";
pipeline.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
_pipeline = device->createGraphicsPipeline(pipeline);
```

The color and depth formats must match the attachments used for rendering.
Setting a depth format enables depth testing in the backend graphics pipeline.
The default topology is `TriangleList`, with the default face-culling settings
from the original GPU example. The vertex buffer description matches the
cube's position/UV layout.

**Reference:** [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 11. Update only the acquired frame slot

```cpp
void update(const draw::FrameContext& context) override
{
    // beginFrame has waited for this slot. Do not overwrite another slot's
    // model buffer while the GPU could still be reading it.
    _time += context.deltaSeconds;
    ModelUBO modelData{};
    modelData.model = glm::rotate(glm::mat4(1.0f), _time,
                                  glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));
    _modelUboRing.at(context.frameSlot)->updateUniformBuffer(modelData);
}
```

Draw acquires a frame before this callback and waits for the selected slot to
be reusable. Updating only that slot's model buffer prevents overwriting data
that another in-flight frame may still read.

`_time` advances only for active scene updates. Pause stops rotation without
jumping to wall-clock time on resume. The same slot is selected when the model
resource set is bound during rendering.

### 12. Render into retained color plus owned depth

```cpp
void render(const draw::FrameContext& context) override
{
    auto& command = context.commandBuffer;
    command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
    command.beginRendering(&context.colorTarget, _depth.get());
    const auto& color = _controller->color;
    command.clearColor(0, {color.r, color.g, color.b, color.a});
    command.clearDepth(1.0f);
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
    command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
    command.bindResourceSet(_pipeline.get(), 2, _textureSet.get());
    _cube.draw(&command);
    command.endRendering();
    ++_controller->sceneFrames;
}
```

Draw supplies the retained color image in `ColorAttachment` layout. The
delegate transitions its depth image, opens a color/depth pass, clears both
attachments, binds the three resource sets, and draws the cube.

The depth clear value is 1.0. Depth testing determines which cube surfaces are
visible as it rotates. The rendering scope is closed before returning; draw
then copies the color result and composes the UI for presentation.

**Reference:** [CommandBuffer](../../api/gpu/CommandBuffer.md), [RenderLoop](../../api/draw/RenderLoop.md)

### 13. Clean up after GPU completion

Draw waits for GPU work before `SceneDelegate::cleanup()`. The callback releases
the graphics pipeline and depth image, cleans and clears both model rings,
and cleans the cube buffers. It then flushes the local cleanup manager to
release the camera/material sets, persistent buffers, texture, sampler, layout,
and shader modules in reverse push order.

The mesh and model rings are handled explicitly because they are containers
rather than individual `DeviceResource` objects. Null checks and empty rings
allow cleanup after partial initialization as well.

## Shader code explanation

### Vertex shader (`cube.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO
{
    mat4 projectionView;
}
camera;

layout(set = 1, binding = 0) uniform ModelUBO
{
    mat4 model;
}
object;

void main()
{
    gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

The vertex shader reads camera and model data from separate uniform blocks,
transforms each local position to clip space, and forwards UV coordinates.

### Fragment shader (`cube.frag.glsl`)

```glsl
#version 450

layout(set = 2, binding = 0) uniform texture2D uTex;
layout(set = 2, binding = 1) uniform sampler uSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

The fragment shader samples the material's separate image and sampler.
It does not need push constants or an animated fragment color: animation comes
from the model matrix in the vertex stage.

### Metal equivalents

`cube.vert.metal` reads `CameraUBO` at `buffer(2)` and `ModelUBO` at `buffer(3)`.
`cube.frag.metal` samples `texture(0)` with `sampler(1)`. The shader logic matches
the GPU example; the copied vertex shader's binding comment is corrected to
reflect the actual buffer indices.

## Key concepts

### Persistent resources and per-slot mutable data

| Resource | Ownership and update strategy |
|----------|-------------------------------|
| Camera UBO/set | Persistent; buffer contents updated only on synchronized resize |
| Model UBO/set | One pair per in-flight frame slot; current slot's buffer updated per active scene frame |
| Texture/sampler/material set | Persistent and unchanged after initialization |
| Depth image | Delegate-owned, matching the current retained target extent |
| Graphics pipeline | Recreated with the current attachment formats on resize |

### Differences from the original GPU example

The original example constructs the GPU context and runs its own SDL frame
loop. This adaptation uses the Application/MainLoop draw path, indexes model
resources explicitly with `FrameContext::frameSlot`, adjusts projection on
resize, and supplies its own depth attachment. It also adds retained pause
behavior and UI controls while preserving the shader resource layout and cube
rotation axis.

The solid background replaces tutorial 02's compute gradient. Camera and model
matrices demonstrate uniform buffer updates rather than fragment push constants.

## Complete render loop flow

```text
initScene: shaders -> layout -> texture/material -> camera -> cube
first drawable or resize:
  wait for prior GPU work (draw)
  release old pipeline/depth/model rings
  update projection -> allocate depth -> create rings -> create pipeline
presentation frame:
  acquire a reusable frame slot and prepare UI (draw)
  if dirty and not paused:
    update the slot's model UBO
    render retained color + owned depth
  copy retained color, compose UI, submit, and present (draw)
shutdown:
  wait for GPU work (draw)
  release scene resources (delegate)
```

## Complete source

See [main.cpp](../../../examples/draw/03_uniform_buffers/src/main.cpp) and the
[shader directory](../../../examples/draw/03_uniform_buffers/shaders/).

## Building and running

The [CMakeLists.txt](../../../examples/draw/03_uniform_buffers/CMakeLists.txt)
uses the same explicit GLSL and Metal shader compilation and packaging helpers
as the GPU example. Compiled resources are packaged under
`shaders/draw_uniform_buffers`.

Use an already configured platform build directory, replacing `build` below
when necessary:

```sh
cmake --build build --target draw_uniform_buffers --config Debug
```

On macOS:

```sh
./bin/macos/draw_uniform_buffers.app/Contents/MacOS/draw_uniform_buffers --backend=metal
./bin/macos/draw_uniform_buffers.app/Contents/MacOS/draw_uniform_buffers --backend=vulkan
```

On Linux, run `./bin/linux/draw_uniform_buffers --backend=vulkan`.
On Windows, run `draw_uniform_buffers.exe` under `bin/windows`.
The macOS target builds Metal shaders even when launching with Vulkan.

To inspect the example:

1. Observe the textured cube rotating with correct surface occlusion.
2. Change Background to verify solid-color scene clearing.
3. Pause and verify that rotation and the scene counter stop while UI frames continue.
4. Change Background while paused, then resume to apply the new color.
5. Resize while active and inspect the updated projection and depth attachment.
6. Resize while paused to observe the replacement retained image's pause clear color; resume to render the cube again.

## Next steps

- [04_render_to_texture](04_render_to_texture.md) — Render the cube offscreen and apply a compute edge filter.

- [Draw tutorial index](index.md) — Follow additions to the example series.
- [GPU Uniform Buffers](../gpu/07_uniform_buffers.md) — Compare the lower-level implementation.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Per-frame resource duplication.
- [Draw API Reference](../../api/draw/index.md) — Delegate lifecycle and frame context contracts.
