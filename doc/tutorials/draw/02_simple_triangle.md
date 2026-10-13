# Tutorial 02: Simple Triangle with Compute and Textured Geometry

This tutorial walks through `02_simple_triangle`, which adapts
[GPU Simple Triangle](../gpu/05_simple_triangle.md) to the `bg2e::draw`
application lifecycle from [01_window_ui](01_window_ui.md). It renders a
textured pentagon over a compute-generated gradient, keeps the information
window, and omits the ImGui demo window.

Despite the example name, the indexed mesh contains five triangles forming a
pentagon. The tutorial follows that geometry from the original GPU example.

**Source:** [examples/draw/02_simple_triangle/src/main.cpp](../../../examples/draw/02_simple_triangle/src/main.cpp)

## What you will learn

- How to create GPU resources in a draw scene delegate
- How to load GLSL and Metal shader modules through `gpu::ShaderLib`
- How to describe graphics and compute resource bindings
- How to upload a texture and build indexed `MeshPU` geometry
- How to generate a gradient in a storage image and sample it in a graphics pass
- How to animate a scene while preserving draw's pause behavior
- How to recreate size-dependent resources and release them safely

## Prerequisites

- Completed [01_window_ui](01_window_ui.md)
- A configured bg2e-native build environment with the required Vulkan SDK
- Application shaders compiled and packaged by the example's CMake target
- On macOS, the Metal compiler toolchain needed for the `.metal` shaders
- Optional: [GPU Simple Triangle](../gpu/05_simple_triangle.md) for the underlying GPU API operations

## Understanding the example

The retained draw target supports color attachment rendering and transfer
reads, but does not support storage writes. The delegate therefore creates a
separate RGBA8 storage/sampled image for compute output.

Each scene render performs these operations:

1. Compute writes a UV gradient to the separate image.
2. The image transitions to `ShaderReadOnly` for fragment sampling.
3. A fullscreen quad draws the gradient into the retained scene target.
4. The same graphics pipeline draws the textured pentagon with animated tint.
5. Draw copies the retained image, composes the UI, and presents the frame.

The Background picker multiplies the gradient's color. White reproduces the
original gradient colors; the default controller color darkens the background.
Pause freezes both the retained scene and the delegate's animation time.

## Step-by-step code explanation

### 1. Reuse the application and UI structure

`Controller`, `Application`, and `main()` follow tutorial 01. The application
registers scene, input, and UI delegates and passes `draw::EngineConfig` to
`MainLoop::run()`. The window title and application identifier use the new
example's name.

The UI keeps Backend, UI frames, Scene updates, Background, Pause scene, and
Refresh scene. It removes `ui::DemoWindow::draw()` and requests a scene frame
on every active UI frame:

```cpp
if (!_controller->paused)
{
    _controller->loop.requestSceneFrame();
}
```

This invalidation makes the animated scene render continuously. While paused,
color changes and refresh requests remain pending until resume, as in tutorial
01. UI counters still advance.

### 2. Create resources during scene initialization

`SceneDelegate` uses the inherited `_engine` pointer to obtain the GPU device
inside `initScene()`. The base delegate's `init()` callback stores that pointer
before scene initialization.

| Callback | Responsibility in this example |
|----------|--------------------------------|
| `initScene()` | Load shaders; create layouts, texture, sampler, sets, meshes, and compute pipeline |
| `resize(extent)` | Create the gradient image, update its bindings, and create the graphics pipeline |
| `update(context)` | Advance scene animation time |
| `render(context)` | Record compute and graphics commands |
| `cleanup()` | Release scene-owned GPU resources |

The engine, surface, and device belong to draw. The delegate only owns the
resources it creates through that device.

**Reference:** [RenderLoopDelegate](../../api/draw/RenderLoopDelegate.md), [Device](../../api/gpu/Device.md)

### 3. Load the shader library

```cpp
gpu::ShaderLib shaders(base::PlatformTools::shaderPath() / "draw_simple_triangle",
                       _engine->backendType());
_vertexShader = shaders.vertex("triangle", device);
_fragmentShader = shaders.fragment("triangle", device);
_computeShader = shaders.compute("gradient", device);
```

ShaderLib resolves backend-specific files below
`PlatformTools::shaderPath() / "draw_simple_triangle"`:

| Stage | Vulkan file | Metal file | Metal entry point |
|-------|-------------|------------|-------------------|
| Vertex | `triangle.vert.spv` | `triangle.vert.metallib` | `vertMain` |
| Fragment | `triangle.frag.spv` | `triangle.frag.metallib` | `fragMain` |
| Compute | `gradient.comp.spv` | `gradient.comp.metallib` | `compMain` |

The current loader requests `main` for Vulkan shader modules. Keep compilation
and loading conventions aligned when introducing new shaders. The shader
modules are retained as delegate members until cleanup.

**Reference:** [ShaderModule](../../api/gpu/ShaderModule.md)

### 4. Define graphics and compute layouts

```cpp
gpu::PipelineLayoutDescription graphicsLayout{};
graphicsLayout.pushConstants.push_back({0, sizeof(PushConstants), gpu::ShaderStage::Fragment});
graphicsLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
graphicsLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
graphicsLayout.debugName = "Textured graphics layout";
_graphicsLayout = device->createPipelineLayout(graphicsLayout);

gpu::PipelineLayoutDescription computeLayout{};
computeLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
computeLayout.debugName = "Gradient compute layout";
_computeLayout = device->createPipelineLayout(computeLayout);
```

The graphics layout has one 16-byte fragment push constant block:

```cpp
struct PushConstants {
    float color[4];
};
```

It also defines a sampled image and a separate sampler. The compute layout
exposes a storage image. These layouts match the shader arguments:

| Pipeline | Set | Vulkan binding | Metal index | Resource | Stage |
|----------|-----|----------------|-------------|----------|-------|
| Graphics | 0 | 0 | texture(0) | Sampled image | Fragment |
| Graphics | 0 | 1 | sampler(1) | Sampler | Fragment |
| Compute | 0 | 0 | texture(0) | Storage image | Compute |

Metal texture, sampler, and buffer indices occupy separate namespaces. Its
fragment push constants use `buffer(0)`, alongside `texture(0)` and `sampler(1)`.

**Reference:** [PipelineLayout](../../api/gpu/PipelineLayout.md), [ResourceSet](../../api/gpu/ResourceSet.md)

### 5. Upload the procedural texture and create resource sets

```cpp
const std::array<std::array<uint8_t, 4>, 4> texels = {{
    {{255, 0, 0, 255}}, {{0, 255, 0, 255}},
    {{0, 0, 255, 255}}, {{255, 255, 0, 255}}
}};
_texture = device->createImage({.size = {2, 2},
                               .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                               .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                               .debugName = "Procedural 2x2 texture"});
_texture->uploadRGBA8(texels.data(), {2, 2});
device->immediateSubmit([this](gpu::CommandBuffer* command)
{
    command->transition(_texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
_sampler = device->createSampler({.debugName = "Linear sampler"});
_textureSet = device->createResourceSet(_graphicsLayout.get(), 0, "Pentagon texture");
_textureSet->setSampledImage({.vulkan = 0, .metal = 0}, _texture.get());
_textureSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
_textureSet->update();
_backgroundSet = device->createResourceSet(_graphicsLayout.get(), 0, "Gradient background");
_computeSet = device->createResourceSet(_computeLayout.get(), 0, "Gradient output");
```

The four RGBA8 texels are red, green, blue, and yellow. `Sampled` permits
fragment reads and `TransferDst` permits upload. After upload, the one-time
submission transitions the image to `ShaderReadOnly`.

The sampler uses the default linear filtering and repeat addressing. The
pentagon's texture set is fully populated once during initialization. Two
additional sets are allocated here and receive their gradient image bindings
in `resize()`:

| Resource set | Image | Sampler | When bindings change |
|--------------|-------|---------|----------------------|
| `_textureSet` | Procedural 2x2 texture | `_sampler` | Initialization |
| `_backgroundSet` | Gradient sampled image | `_sampler` | Resize |
| `_computeSet` | Gradient storage image | None | Resize |

**Reference:** [Image](../../api/gpu/Image.md), [ResourceSet](../../api/gpu/ResourceSet.md)

### 6. Build the pentagon and fullscreen quad

The pentagon uses `geo::MeshPU` with six position/UV vertices, fifteen indices,
and one submesh. Its five triangles share the center vertex at index 5:

```cpp
meshData.indices = {
    5, 0, 1,
    5, 1, 2,
    5, 2, 3,
    5, 3, 4,
    5, 4, 0
};
meshData.submeshes = {{0, 15}};
_mesh.setMeshData(meshData);
_mesh.build(device);
```

The fullscreen background uses the same vertex format and shader pair:

```cpp
geo::MeshPU background;
background.vertices = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}}
};
background.indices = {0, 1, 2, 0, 2, 3};
background.submeshes = {{0, 6}};
_backgroundMesh.setMeshData(background);
_backgroundMesh.build(device);
```

Both meshes upload persistent vertex and index buffers. The graphics pipeline
can draw either mesh by binding a different image set and pushing a different
color. `MeshPU` supplies position at location 0 and UV at location 1.

**Reference:** [Mesh](../../api/gpu/Mesh.md)

### 7. Create the compute pipeline

```cpp
gpu::ComputePipelineDescription computePipeline{};
computePipeline.computeShader = _computeShader.get();
computePipeline.layout = _computeLayout.get();
computePipeline.debugName = "Gradient compute pipeline";
_computePipeline = device->createComputePipeline(computePipeline);
```

The compute pipeline only needs its shader module and layout. Its output image
is provided later by `_computeSet`; the pipeline itself is independent of the
window dimensions.

**Reference:** [ComputePipeline](../../api/gpu/ComputePipeline.md)

### 8. Recreate resources for the current extent

```cpp
void resize(gpu::Size2D extent) override
{
    // draw drains in-flight frames before resize. Descriptor bindings remain
    // immutable between resizes, so one persistent set can serve every slot.
    if (_gradient)
    {
        _gradient->cleanup();
    }
    auto* device = _engine->device();
    _gradient = device->createImage({.size = extent,
                                    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                    .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled,
                                    .debugName = "Compute gradient"});
    _computeSet->setStorageImage({.vulkan = 0, .metal = 0}, _gradient.get());
    _computeSet->update();
    _backgroundSet->setSampledImage({.vulkan = 0, .metal = 0}, _gradient.get());
    _backgroundSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
    _backgroundSet->update();

    // Recreate for the current color format. draw's retained target has no depth.
    if (_pipeline)
    {
        _pipeline->cleanup();
    }
    gpu::GraphicsPipelineDescription pipeline{};
    pipeline.vertexShader = _vertexShader.get();
    pipeline.fragmentShader = _fragmentShader.get();
    pipeline.layout = _graphicsLayout.get();
    pipeline.colorFormat = _engine->surface()->colorFormat();
    pipeline.cullMode = gpu::CullMode::None;
    pipeline.debugName = "Textured background and pentagon";
    pipeline.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    _pipeline = device->createGraphicsPipeline(pipeline);
}
```

Draw calls `resize()` when it creates or replaces the retained target, including
before the first scene render. It drains in-flight GPU work before this callback,
so the delegate can replace the old gradient image and descriptor bindings.

The gradient has `R8G8B8A8_UNORM` format, matching the compute shader's `rgba8`
image declaration, and combines `Storage` and `Sampled` usage. The background
pass can sample it even when the retained target uses a different color format.

The graphics pipeline uses the surface's current color format and
`MeshPU::vertexBufferDescription()`. `depthFormat` remains `Undefined`, because
the retained draw target has no depth attachment. Culling is disabled for these
two-dimensional meshes. The background draws first, followed by the pentagon.

**Reference:** [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md), [FrameContext](../../api/draw/FrameContext.md)

### 9. Advance animation only for active scene updates

```cpp
void update(const draw::FrameContext& context) override
{
    // Scene time advances only while active; pause also freezes the tint.
    _time += context.deltaSeconds;
}
```

Draw invokes `update()` when the scene is dirty and not paused. The accumulated
time therefore stops during pause. The current presentation frame interval is
expressed in seconds, so it can be used directly by the sine-wave tint.

### 10. Record the compute and graphics passes

```cpp
void render(const draw::FrameContext& context) override
{
    auto& command = context.commandBuffer;
    command.transition(_gradient.get(), gpu::ImageLayout::General);
    command.beginCompute();
    command.bindPipeline(_computePipeline.get());
    command.bindResourceSet(_computePipeline.get(), 0, _computeSet.get());
    command.dispatch((context.extent.width + 15) / 16,
                     (context.extent.height + 15) / 16, 1);
    command.endCompute();
    command.transition(_gradient.get(), gpu::ImageLayout::ShaderReadOnly);

    command.beginRendering(&context.colorTarget);
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, _backgroundSet.get());
    const auto& color = _controller->color;
    const PushConstants background{{color.r, color.g, color.b, color.a}};
    command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(background), &background);
    _backgroundMesh.draw(&command);

    const PushConstants tint{{
        0.5f + 0.5f * std::sin(_time * 1.3f),
        0.5f + 0.5f * std::sin(_time * 1.7f + 1.0f),
        0.5f + 0.5f * std::sin(_time * 2.1f + 2.0f),
        1.0f
    }};
    command.bindResourceSet(_pipeline.get(), 0, _textureSet.get());
    command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(tint), &tint);
    _mesh.draw(&command);
    command.endRendering();
    ++_controller->sceneFrames;
}
```

The transition to `General` prepares the gradient for compute writes.
Dispatch rounds the extent up to groups of 16 by 16 threads; the shader checks
bounds for the partial groups. Closing compute and transitioning to
`ShaderReadOnly` orders the writes before fragment sampling.

The graphics pass targets `context.colorTarget`. It draws the background with
the controller's color, then binds the procedural texture and draws the
pentagon with three animated sine-wave color channels. Both draws reuse the
same pipeline and push constant layout.

The delegate closes rendering and increments the scene counter. Draw handles
the retained-image copy, UI composition, queue submission, and presentation.

**Reference:** [CommandBuffer](../../api/gpu/CommandBuffer.md), [RenderLoop](../../api/draw/RenderLoop.md)

### 11. Release resources after GPU completion

The delegate's `cleanup()` releases both meshes, the pipelines, all three
resource sets, the sampler, the two images, the layouts, and the shader modules.
Draw waits for GPU work to complete before invoking delegate cleanup.

The `release()` helper checks whether each shared resource exists, calls its
`cleanup()`, and resets the pointer. This also supports shutdown after partially
completed initialization. The delegate does not destroy draw's device or surface.

## Shader code explanation

The six shader source files are copied from the GPU example. Their bindings
and logic work with the gradient image owned by this delegate.

### Vertex shader (`triangle.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

The vertex shader passes the mesh position directly to clip space and forwards
UV coordinates. The same shader serves the pentagon and fullscreen quad.

### Fragment shader (`triangle.frag.glsl`)

```glsl
#version 450

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler uSampler;

layout(push_constant) uniform Push
{
    vec4 color;
}
pc;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 tex = texture(sampler2D(uTex, uSampler), fragUV);
    outColor = vec4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}
```

The shader samples a separate texture/sampler pair and multiplies RGBA by the
push constant color. For the quad, that color is the selected background tint;
for the pentagon, it is the animated tint.

### Compute shader (`gradient.comp.glsl`)

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform image2D outImage;

void main()
{
    ivec2 size = imageSize(outImage);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= size.x || pos.y >= size.y)
    {
        return;
    }
    vec2 uv = vec2(float(pos.x) / float(size.x), float(pos.y) / float(size.y));
    imageStore(outImage, pos, vec4(uv.x, uv.y, 0.5, 1.0));
}
```

Each invocation writes `(u, v, 0.5, 1.0)` into one pixel. Bounds checks prevent
out-of-range writes when the image dimensions are not multiples of 16.

### Metal equivalents

The `.metal` files implement the same vertex attributes, sampled texture,
sampler, push constant color, and compute gradient. Their functions are
`vertMain`, `fragMain`, and `compMain`. The compute kernel uses
`thread_position_in_grid` and a write-access texture at `texture(0)`.

## Key concepts

### Retained scene rendering changes where compute writes

| Operation | GPU example 05 | Draw example 02 |
|-----------|----------------|-----------------|
| GPU context setup | Explicit backend, instance, surface, and device setup | Managed by Application/MainLoop and draw |
| Compute output | Acquired presentation color image | Separate RGBA8 storage/sampled image |
| Background | Compute writes directly into the output | Graphics samples compute output with a fullscreen quad |
| Graphics attachments | Surface color and depth | Retained scene color, without depth |
| Compute descriptor bindings | Updated for acquired frame images | Fixed between resizes |
| Animation time | SDL elapsed time | Accumulated active scene delta time |
| Presentation | Explicit submission and surface frame lifecycle | Managed by draw |

### Stable descriptor bindings and shared-image synchronization

The compute and background sets always point to the same gradient image between
resizes. They can be reused across frame slots without descriptor updates during
rendering. Compute and sampling execute on the graphics queue, and explicit
image transitions order access to the shared image between passes and frames.

This does not mean all mutable resources should be shared. Future resources
whose descriptor contents or CPU-visible data change while frames remain in
flight should use a suitable per-frame-slot allocation strategy.

### Pause and resize

Pause keeps the last valid retained scene, stops scene updates, and leaves the
UI active. Resizing while paused replaces that retained image with a clear
using the color previously passed to `pauseScene()`. The newly created gradient
is written on the next active scene render after resume.

## Complete render loop flow

```text
Application/MainLoop starts the draw execution path
  -> SceneDelegate::initScene creates persistent resources
  -> draw creates a retained target and calls resize(extent)
  -> draw acquires a presentation frame and prepares the UI
  -> active UI requests a scene frame
  -> if the scene is dirty and not paused:
       update: advance animation time
       render: compute gradient -> sample background -> draw pentagon
  -> draw copies the retained scene to the presentation image
  -> draw composes the UI, submits commands, and presents
  -> shutdown waits for GPU work and calls delegate cleanup
```

## Building and running

The example's
[CMakeLists.txt](../../../examples/draw/02_simple_triangle/CMakeLists.txt)
uses the same explicit shader compilation and resource packaging calls as
`gpu/05_simple_triangle`: `compile_shaders_shaderlib()` for GLSL,
`compile_metal_shaders()` on macOS, and `bundle_resources()` for both outputs.
The compiled files are packaged under `shaders/draw_simple_triangle`.

Use your configured platform build directory; replace `build` when necessary:

```sh
cmake --build build --target draw_simple_triangle --config Debug
```

On macOS:

```sh
./bin/macos/draw_simple_triangle.app/Contents/MacOS/draw_simple_triangle --backend=metal
./bin/macos/draw_simple_triangle.app/Contents/MacOS/draw_simple_triangle --backend=vulkan
```

On Linux, run `./bin/linux/draw_simple_triangle --backend=vulkan`.
On Windows, run `draw_simple_triangle.exe` under `bin/windows`.
The default backend and argument validation match tutorial 01. On macOS the
build includes Metal shaders even if you plan to launch with the Vulkan backend.

To inspect the result:

1. Observe the changing tint on the textured pentagon.
2. Set Background to white to view the original compute gradient colors.
3. Enable Pause scene and verify that the image and scene counter stop changing.
4. Change Background while paused; resume to apply the new tint.
5. Resize the window while active to recreate the gradient at the new extent.

## Next steps

- [03_uniform_buffers](03_uniform_buffers.md) — Add camera/model uniform buffers and depth-tested cube rendering.

- [Draw tutorial index](index.md) — New tutorials will be added as the example series grows.
- [GPU Simple Triangle](../gpu/05_simple_triangle.md) — Compare explicit GPU lifecycle code.
- [Draw API Reference](../../api/draw/index.md) — Explore engine and frame contracts.
- [GPU API Reference](../../api/gpu/index.md) — Explore pipelines, images, meshes, and resource sets.
