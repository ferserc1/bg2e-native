# Tutorial 04: Render to Texture with a Compute Edge Filter

This tutorial adapts [GPU Render to Texture](../gpu/08_render_to_texture.md)
to `bg2e::draw`. It renders a rotating textured cube into an offscreen image,
processes the result with a Sobel compute filter, copies the processed pixels
between images, and samples them into draw's retained scene target.

**Source:** [examples/draw/04_render_to_texture/src/main.cpp](../../../examples/draw/04_render_to_texture/src/main.cpp)

## What you will learn

- How to render into delegate-owned color and depth images
- How to bind separate compute input and output storage images
- How to order render, compute, copy, and sampling operations with transitions
- How to copy images with matching dimensions and formats
- How to display postprocessed pixels in draw's retained color target
- How to handle Sobel neighborhood reads at image borders
- How to retain animation, pause, and resize behavior across multiple passes

## Prerequisites

- Completed [03_uniform_buffers](03_uniform_buffers.md)
- A configured bg2e-native build environment with the required Vulkan SDK
- The example's GLSL and, on macOS, Metal shaders compiled and packaged
- On macOS, the Metal compiler toolchain
- Optional: [GPU Render to Texture](../gpu/08_render_to_texture.md) for the original lower-level example

## Understanding the example

Tutorial 03 renders the cube directly into retained color with its own depth
attachment. This example instead renders into an RGBA8 offscreen image and
runs a compute filter before updating the retained target.

Each active scene frame has four stages:

1. Render the cube into `_offscreenColor` with `_depth`.
2. Read `_offscreenColor` and write grayscale Sobel edges to `_computeOutput`.
3. Copy `_computeOutput` back into `_offscreenColor` after compute finishes.
4. Sample the copied result with a fullscreen triangle into `context.colorTarget`.

The original unfiltered color is no longer needed after compute. Reusing its
storage as the copy destination keeps the example at three size-dependent
images: color, depth, and compute output.

Draw's retained target is created with `ColorAttachment | TransferSrc` usage.
It cannot serve as a transfer destination or storage image. The final sampling
pass also handles the difference between RGBA8 processing images and the
retained target's format, which is commonly BGRA8.

The UI keeps the backend label, scene/UI counters, Background, Pause scene,
and Refresh scene. Background controls the cube pass's clear color, so it
changes edge contrast against the cube. Uniform regions remain dark after
Sobel; the picker does not tint the final grayscale image.

## Step-by-step code explanation

### 1. Reuse the cube, UBOs, and application lifecycle

`CameraUBO`, `ModelUBO`, the procedural cube, material texture, sampler, and
camera/model resource sets follow tutorial 03:

| Cube set | Resource | Metal index |
|----------|----------|-------------|
| 0, binding 0 | Camera UBO | `buffer(2)` |
| 1, binding 0 | Model UBO | `buffer(3)` |
| 2, binding 0 | Sampled texture | `texture(0)` |
| 2, binding 1 | Sampler | `sampler(1)` |

Camera contents change only during synchronized resize. Model buffers and
sets use matching `FrameResourceRing` entries selected by `context.frameSlot`.
Active scene delta time drives rotation, so pause also freezes the cube angle.

Application/MainLoop owns the draw engine and frame lifecycle. The UI requests
scene frames while active; there is no private SDL rendering loop or direct
Factory initialization in this example.

**Reference:** [FrameResourceRing](../../api/gpu/FrameResourceRing.md), [RenderLoopDelegate](../../api/draw/RenderLoopDelegate.md)

### 2. Load cube, filter, and output shaders

`gpu::ShaderLib` uses the directory
`PlatformTools::shaderPath() / "draw_render_to_texture"`. The delegate loads:

| Shader name | Stages | Purpose |
|-------------|--------|---------|
| `cube` | Vertex, fragment | UBO-transformed textured cube |
| `edge_filter` | Compute | Sobel edge detection |
| `output` | Vertex, fragment | Fullscreen display of the copied result |

All modules are tracked by a local `CleanupManager`. The cube shader logic is
inherited from the GPU example. The filter adds clamped neighborhood reads;
the output shader pair is added for retained-target composition.

### 3. Create the compute layout and pipeline

```cpp
gpu::PipelineLayoutDescription computeLayout{};
computeLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
computeLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1});
computeLayout.debugName = "Sobel compute layout";
_computeLayout = device->createPipelineLayout(computeLayout);
_cleanup->push(_computeLayout);
gpu::ComputePipelineDescription computePipeline{};
computePipeline.computeShader = _computeShader.get();
computePipeline.layout = _computeLayout.get();
computePipeline.debugName = "Sobel compute pipeline";
_computePipeline = device->createComputePipeline(computePipeline);
_cleanup->push(_computePipeline);
```

Compute uses set 0 with storage images at bindings 0 and 1. Metal maps these
to `texture(0)` and `texture(1)`. Input and output are different images, so
neighbor reads observe the completed cube render rather than partially
filtered pixels being overwritten by other compute invocations.

The compute pipeline is persistent because its format declaration and shader
are independent of the window extent.

**Reference:** [ComputePipeline](../../api/gpu/ComputePipeline.md), [PipelineLayout](../../api/gpu/PipelineLayout.md)

### 4. Create the fullscreen output layout

```cpp
gpu::PipelineLayoutDescription outputLayout{};
outputLayout.resourceBindings.push_back({0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1});
outputLayout.resourceBindings.push_back({0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1});
outputLayout.debugName = "Fullscreen output layout";
_outputLayout = device->createPipelineLayout(outputLayout);
_cleanup->push(_outputLayout);
```

This separate graphics layout exposes one sampled image and one sampler at
set 0. It needs no UBOs or push constants: the vertex shader generates its
own fullscreen triangle.

### 5. Allocate offscreen images and bind them on resize

```cpp
// The cube and Sobel pass use RGBA8, independent of the retained
// target's format. Reuse the color image for the final sampled result.
_offscreenColor = device->createImage({.size = extent,
                                      .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                      .usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Storage |
                                               gpu::ImageUsage::TransferDst | gpu::ImageUsage::Sampled,
                                      .debugName = "Offscreen cube color and copied result"});
_depth = device->createImage({.size = extent,
                             .format = gpu::PixelFormat::D32_SFLOAT,
                             .usage = gpu::ImageUsage::DepthStencil,
                             .debugName = "Offscreen cube depth"});
_computeOutput = device->createImage({.size = extent,
                                     .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                                     .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc,
                                     .debugName = "Sobel output"});
_computeSet = device->createResourceSet(_computeLayout.get(), 0, "Sobel images");
_computeSet->setStorageImage({.vulkan = 0, .metal = 0}, _offscreenColor.get());
_computeSet->setStorageImage({.vulkan = 1, .metal = 1}, _computeOutput.get());
_computeSet->update();
_outputSet = device->createResourceSet(_outputLayout.get(), 0, "Fullscreen output image");
_outputSet->setSampledImage({.vulkan = 0, .metal = 0}, _offscreenColor.get());
_outputSet->setSampler({.vulkan = 1, .metal = 1}, _sampler.get());
_outputSet->update();
```

All images use the actual drawable extent provided by draw. Their usage flags
cover the operations performed by the delegate:

| Image | Format | Usage | Role |
|-------|--------|-------|------|
| `_offscreenColor` | RGBA8 UNORM | ColorAttachment, Storage, TransferDst, Sampled | Rendered cube, compute input, then copied result |
| `_depth` | D32 float | DepthStencil | Cube visibility |
| `_computeOutput` | RGBA8 UNORM | Storage, TransferSrc | Sobel destination and copy source |

RGBA8 matches the filter's `rgba8` image declarations. Both sides of the image
copy have identical size and format. No direct RGBA-to-BGRA image copy is used.

`_computeSet` binds the color image for compute reads and the output image for
compute writes. `_outputSet` samples the color image after the processed copy.
These bindings stay unchanged between resizes.

**Reference:** [Image](../../api/gpu/Image.md), [ResourceSet](../../api/gpu/ResourceSet.md)

### 6. Create pipelines for their respective attachments

The cube pipeline uses `_offscreenColor->pixelFormat()` and
`_depth->pixelFormat()`, together with `MeshPU::vertexBufferDescription()`.
It retains depth testing and the cube shader bindings from tutorial 03.

The output pipeline targets the current surface color format:

```cpp
gpu::GraphicsPipelineDescription outputPipeline{};
outputPipeline.vertexShader = _outputVertexShader.get();
outputPipeline.fragmentShader = _outputFragmentShader.get();
outputPipeline.layout = _outputLayout.get();
outputPipeline.colorFormat = surface->colorFormat();
outputPipeline.cullMode = gpu::CullMode::None;
outputPipeline.debugName = "Fullscreen processed output";
_outputPipeline = device->createGraphicsPipeline(outputPipeline);
```

It has no depth format or vertex buffer descriptions. `draw(3)` uses a vertex
index to produce three clip-space positions that cover the viewport. Culling
is disabled so the fullscreen draw does not depend on winding.

**Reference:** [GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 7. Update the current slot's model transform

`update()` accumulates `context.deltaSeconds`, constructs the same tilted-axis
rotation as the previous example, and updates only
`_modelUboRing.at(context.frameSlot)`. Draw has already waited for that slot to
be reusable before invoking the callback.

The retained image and animation time remain unchanged while paused. A color
change or Refresh scene request stays pending until resume.

### 8. Record all four stages

```cpp
void render(const draw::FrameContext& context) override
{
    auto& command = context.commandBuffer;
    // Pass 1: render the rotating cube into a separate RGBA8 image.
    command.transition(_offscreenColor.get(), gpu::ImageLayout::ColorAttachment);
    command.transition(_depth.get(), gpu::ImageLayout::DepthAttachment);
    command.beginRendering(_offscreenColor.get(), _depth.get());
    const auto& color = _controller->color;
    command.clearColor(0, {color.r, color.g, color.b, color.a});
    command.clearDepth(1.0f);
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
    command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
    command.bindResourceSet(_pipeline.get(), 2, _textureSet.get());
    _cube.draw(&command);
    command.endRendering();

    // Pass 2: Sobel reads the rendered image and writes a separate output.
    command.transition(_offscreenColor.get(), gpu::ImageLayout::General);
    command.transition(_computeOutput.get(), gpu::ImageLayout::General);
    command.beginCompute();
    command.bindPipeline(_computePipeline.get());
    command.bindResourceSet(_computePipeline.get(), 0, _computeSet.get());
    command.dispatch((context.extent.width + 15) / 16,
                     (context.extent.height + 15) / 16, 1);
    command.endCompute();

    // Pass 3: demonstrate a same-format image copy. The unfiltered color
    // is no longer needed, so reuse its storage instead of a fourth image.
    command.transition(_computeOutput.get(), gpu::ImageLayout::TransferSrc);
    command.transition(_offscreenColor.get(), gpu::ImageLayout::TransferDst);
    command.copyImage(_computeOutput.get(), _offscreenColor.get());

    // Pass 4: sample into draw's retained color target. That target has
    // no TransferDst usage and may have a different format (e.g. BGRA8).
    command.transition(_offscreenColor.get(), gpu::ImageLayout::ShaderReadOnly);
    command.beginRendering(&context.colorTarget);
    command.bindPipeline(_outputPipeline.get());
    command.bindResourceSet(_outputPipeline.get(), 0, _outputSet.get());
    command.draw(3);
    command.endRendering();
    ++_controller->sceneFrames;
}
```

The stages share a single borrowed command buffer, and every rendering or
compute scope is closed before opening the next one.

The first transition prepares color attachment writes. After cube rendering,
`General` permits storage-image reads and writes. After compute, transfer
layouts order the Sobel output before the copy. Finally, `ShaderReadOnly`
orders the copy before sampling in the fullscreen pass.

The retained target is already in `ColorAttachment` layout when the delegate
starts. The final pass covers it completely and returns without an active
scope. Draw then performs its retained-image copy to the presentation image,
composes the UI, submits, and presents.

**Reference:** [CommandBuffer](../../api/gpu/CommandBuffer.md), [FrameContext](../../api/draw/FrameContext.md)

### 9. Recreate resources and clean up safely

Before the first render and on resize, draw drains prior GPU work and calls
`resize(extent)`. The delegate releases the two graphics pipelines, the compute
and output resource sets, and the three offscreen images. It also cleans and
clears the model rings.

The camera projection is updated from the new aspect ratio, then images, sets,
model rings, and graphics pipelines are rebuilt. Persistent texture, shader,
layout, camera, and compute-pipeline objects are retained.

Shutdown uses the same ordering after draw waits for completion. Target resources
and model rings are cleaned explicitly, followed by the cube buffers. The local
cleanup manager then flushes its persistent resources in reverse push order.
Partial initialization is handled with null checks and empty-ring cleanup.

## Shader code explanation

### Cube shaders

`cube.vert.glsl` transforms positions with
`camera.projectionView * object.model`; `cube.frag.glsl` samples the material
texture. Metal uses the same UBO matrices at `buffer(2)` and `buffer(3)`.
See [tutorial 03](03_uniform_buffers.md) for their complete explanation.

### Compute shader (`edge_filter.comp.glsl`)

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputImage;
layout(set = 0, binding = 1, rgba8) uniform image2D outputImage;

float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float lumaAt(ivec2 pos)
{
    ivec2 clampedPos = clamp(pos, ivec2(0), imageSize(inputImage) - ivec2(1));
    return luma(imageLoad(inputImage, clampedPos).rgb);
}

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (pos.x >= size.x || pos.y >= size.y)
    {
        return;
    }

    // Sample 3x3 neighborhood luminances
    float tl = lumaAt(pos + ivec2(-1, -1));
    float t = lumaAt(pos + ivec2(0, -1));
    float tr = lumaAt(pos + ivec2(1, -1));
    float l = lumaAt(pos + ivec2(-1, 0));
    float r = lumaAt(pos + ivec2(1, 0));
    float bl = lumaAt(pos + ivec2(-1, 1));
    float b = lumaAt(pos + ivec2(0, 1));
    float br = lumaAt(pos + ivec2(1, 1));

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0, 1.0);

    imageStore(outputImage, pos, vec4(vec3(edge), 1.0));
}
```

The Sobel filter converts eight neighboring pixels to luminance, evaluates
horizontal and vertical gradients, and writes the clamped gradient magnitude
as grayscale with alpha 1. The luminance weights are 0.299, 0.587, and 0.114.

A uniform background has zero gradient and becomes black. Visible edges arise
from luminance differences across the cube silhouette and texture boundaries.

Two bounds protections serve different purposes:

- The invocation guard skips threads beyond the image extent in partial workgroups.
- `lumaAt()` clamps every neighborhood coordinate to the nearest valid pixel,
  including on the first and last rows and columns. It also supports images
  smaller than the 3x3 neighborhood.

The copied GPU example only checks invocation coordinates. This adaptation
adds neighborhood clamping in both GLSL and Metal so border reads are defined.

### Fullscreen vertex shader (`output.vert.glsl`)

```glsl
#version 450

layout(location = 0) out vec2 fragUV;

void main()
{
    const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 position = positions[gl_VertexIndex];
    gl_Position = vec4(position, 0.0, 1.0);
    fragUV = vec2(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);
}
```

The oversized triangle extends beyond the viewport and is clipped to its
bounds. UV coordinates map the image's top row to the top of the output,
following the same orientation as the fullscreen quad in tutorial 02.

### Fullscreen fragment shader (`output.frag.glsl`)

```glsl
#version 450

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler uSampler;
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

Sampling retrieves the copied RGBA pixels; the graphics attachment handles
writing them in the retained target's native color format. The existing linear
sampler is reused.

### Metal equivalents

The compute entry point is `compMain`, reading `texture(0)` and writing
`texture(1)`. Its `lumaAt()` helper clamps signed coordinates before converting
them to unsigned texture positions. The output vertex shader uses `vertex_id`
and the output fragment shader uses `texture(0)` with `sampler(1)`.
All graphics entry points are `vertMain` and `fragMain`.

## Key concepts

### Image layouts follow each role

| Image | Layout sequence for an active scene frame |
|-------|-------------------------------------------|
| Offscreen color | ColorAttachment -> General -> TransferDst -> ShaderReadOnly |
| Offscreen depth | DepthAttachment |
| Compute output | General -> TransferSrc |
| Retained color | ColorAttachment during the final pass; draw then handles its transfer/presentation work |

Transitions also synchronize access between consecutive frames on the same
graphics queue. Descriptor contents do not change during rendering; only the
model UBO contents for the reusable slot are updated on the CPU.

### Copying preserves the original example's GPU operation

The GPU example copies the processed image to an acquired presentation image.
This adaptation copies between two owned RGBA8 images, then samples into the
retained target. The intermediate copy demonstrates `copyImage()` with matching
formats and usage flags while respecting draw's target contract.

Reusing the offscreen color image after compute is safe because all compute
reads finish before the transfer writes overwrite it. The next active scene
frame clears and renders new cube pixels into the same storage.

### Retained pause and UI composition

Pause freezes the full processed result and scene animation while the UI keeps
updating. Resizing while paused replaces the retained image with draw's pause
clear color; the offscreen passes execute again after resume. The UI is composed
after filtering, so text and controls are not affected by Sobel.

## Complete render loop flow

```text
initScene: cube resources -> compute pipeline -> output layout/shaders
first drawable or resize:
  drain prior GPU work (draw)
  update camera -> create color/depth/output images -> bind sets
  rebuild model rings and cube/output graphics pipelines
active scene frame:
  update current frame-slot model UBO
  render cube into RGBA8 color/depth
  compute Sobel into separate RGBA8 output
  copy Sobel output back into the offscreen color image
  sample copied pixels into draw's retained color target
presentation:
  copy retained color -> compose UI -> submit -> present (draw)
shutdown:
  drain GPU work -> clean target resources/rings/mesh -> flush persistent resources
```

## Complete source

See [main.cpp](../../../examples/draw/04_render_to_texture/src/main.cpp) and the
[shader directory](../../../examples/draw/04_render_to_texture/shaders/).

## Building and running

The [CMakeLists.txt](../../../examples/draw/04_render_to_texture/CMakeLists.txt)
uses the same shader compilation and packaging calls as the source GPU example:
`compile_shaders_shaderlib()`, `compile_metal_shaders()` on macOS, and
`bundle_resources()`. The compiled shaders are packaged below
`shaders/draw_render_to_texture`.

Use your already configured platform build directory, replacing `build` below
when necessary:

```sh
cmake --build build --target draw_render_to_texture --config Debug
```

On macOS:

```sh
./bin/macos/draw_render_to_texture.app/Contents/MacOS/draw_render_to_texture --backend=metal
./bin/macos/draw_render_to_texture.app/Contents/MacOS/draw_render_to_texture --backend=vulkan
```

On Linux, run `./bin/linux/draw_render_to_texture --backend=vulkan`.
On Windows, run `draw_render_to_texture.exe` under `bin/windows`.
Backend defaults and argument validation match the earlier draw examples.
The macOS target includes Metal compilation even when launching with Vulkan.

To inspect the result:

1. Observe the rotating cube's grayscale silhouette and texture edges.
2. Change Background to vary the input contrast; flat regions remain black.
3. Pause and verify that processed pixels and Scene updates stop changing while UI frames continue.
4. Change Background or request refresh while paused, then resume to apply it.
5. Resize while active, including to dimensions not divisible by 16, to inspect resource recreation and dispatch bounds.
6. Resize while paused, then resume to restore the processed cube at the new extent.

## Next steps

- [05_cubemap](05_cubemap.md) — Convert an HDR environment to a cubemap and render a mirror cube.

- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Render to Texture](../gpu/08_render_to_texture.md) — Compare explicit GPU frame management.
- [CommandBuffer](../../api/gpu/CommandBuffer.md) — Rendering, compute dispatch, transitions, and image copying.
- [Draw API Reference](../../api/draw/index.md) — Retained targets and scene callback contracts.
