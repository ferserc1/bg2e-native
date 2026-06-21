# Tutorial 08: Render to Texture

This tutorial walks through the `08_render_to_texture` example: rendering a rotating cube into an offscreen color image, running a compute Sobel edge-detection pass, then copying the result to the surface for presentation. This is the foundational example for multi-pass rendering using the GPU abstraction layer.

**Source:** `examples/gpu/08_render_to_texture/src/main.cpp`

## What you will learn

- How to create offscreen `Image` objects for color and depth rendering
- How to use `beginRendering(Image*, Image*)` for offscreen render passes
- How to set up a compute pipeline with storage image inputs and outputs
- How to use `image2D`, `imageLoad()`, and `imageStore()` in compute shaders
- How to chain three GPU passes in a single command buffer: render, compute, copy
- How to handle `ImageLayout` transitions across the full render-compute-present pipeline
- How to use `copyImage()` to transfer between `gpu::Image` objects
- How to recreate offscreen images on window resize

## Prerequisites

- Completed [07_uniform_buffers](07_uniform_buffers.md) -- you should understand pipeline creation, descriptor sets, and uniform buffers
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding render-to-texture

Render-to-texture is a technique where the GPU renders a scene into an offscreen image instead of directly into the swapchain framebuffer. The rendered image can then be post-processed, combined with other textures, or copied to the surface for presentation.

This example implements a three-pass pipeline:

```
Pass 1: Graphics     Pass 2: Compute       Pass 3: Copy
 ┌────────────┐     ┌────────────┐       ┌────────────┐
 │ Render cube│────>│ Sobel edge │──────>│ Copy result│──> Surface
 │ into off-  │     │ detection  │       │ to present │
 │ screen     │     │            │       │ image      │
 │ color+depth│     └────────────┘       └────────────┘
 └────────────┘
```

Each pass requires different image layouts:

| Pass | Input layout | Output layout | Operation |
|------|-------------|---------------|-----------|
| 1 | ColorAttachment, DepthAttachment | ShaderReadOnly (for compute read) | Graphics draw |
| 2 | General (compute read/write) | TransferSrc (for copy) | Compute dispatch |
| 3 | TransferDst (copy target) | Present | Image copy |

**Reference:** [GPU API -- Image](../../api/gpu/Image.md), [GPU API -- CommandBuffer](../../api/gpu/CommandBuffer.md)

## Step-by-step code explanation

### 1. Helper functions for offscreen images

```cpp
static void createOffscreenImages(
    bg2e::gpu::Device* device,
    const bg2e::gpu::Size2D& size,
    std::shared_ptr<bg2e::gpu::Image>& offscreenColor,
    std::shared_ptr<bg2e::gpu::Image>& offscreenDepth,
    std::shared_ptr<bg2e::gpu::Image>& computeOutput,
    const std::string& tag)
{
    using namespace bg2e;

    offscreenColor = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::Sampled
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::Storage,
        .debugName = "Offscreen color " + tag
    });

    offscreenDepth = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::D32_SFLOAT,
        .usage = gpu::ImageUsage::DepthStencil
               | gpu::ImageUsage::Sampled,
        .debugName = "Offscreen depth " + tag
    });

    computeOutput = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::Storage
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst
               | gpu::ImageUsage::Storage,
        .debugName = "Compute output " + tag
    });
}
```

This helper creates the three offscreen images needed by the pipeline. Each image is created with specific usage flags that determine what operations the GPU can perform on it:

| Image | Format | Usage flags | Purpose |
|-------|--------|-------------|---------|
| `offscreenColor` | R8G8B8A8_UNORM | ColorAttachment, Sampled, TransferSrc, Storage | Render target for the cube; sampled by compute shader |
| `offscreenDepth` | D32_SFLOAT | DepthStencil, Sampled | Depth buffer for 3D rendering |
| `computeOutput` | R8G8B8A8_UNORM | Storage, TransferSrc, TransferDst, Storage | Compute shader writes edge-detected result here |

The `offscreenColor` image needs `ColorAttachment` (for the graphics pass), `Sampled` (to be read as input by the compute shader), `TransferSrc` (for `copyImage`), and `Storage` (for compute read access). The `computeOutput` image needs `Storage` (compute writes), `TransferSrc` and `TransferDst` (for `copyImage` to the surface).

**Reference:** [GPU API -- Image](../../api/gpu/Image.md)

### 2. Helper for compute resource sets

```cpp
static std::shared_ptr<bg2e::gpu::ResourceSet> createComputeResourceSet(
    bg2e::gpu::Device* device,
    bg2e::gpu::PipelineLayout* layout,
    bg2e::gpu::Image* inputImage,
    bg2e::gpu::Image* outputImage)
{
    auto set = device->createResourceSet(layout, 0, "Compute resource set");
    set->setStorageImage({.vulkan = 0, .metal = 0}, inputImage);
    set->setStorageImage({.vulkan = 1, .metal = 1}, outputImage);
    set->update();
    return set;
}
```

This helper creates a resource set for the compute pipeline, binding two storage images:

- **Binding 0** (`inputImage`): the offscreen color image that the compute shader reads from
- **Binding 1** (`outputImage`): the image the compute shader writes edge-detected pixels into

Both are bound with `setStorageImage()`, which is the method for binding `image2D` resources that shaders can read and write via `imageLoad()`/`imageStore()`.

The set is extracted into a helper because it must be recreated whenever the offscreen images change (on window resize).

**Reference:** [GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)

### 3. Device initialization and shader creation

```cpp
auto backendType = gpu::BackendType::Vulkan;
// ... platform check for backend selection ...

gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
// ... SDL init, window creation, instance, surface, physical device, device ...

gpu::CleanupManager cleanup(surface.get());

auto shaderBasePath = base::PlatformTools::shaderPath();
auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_render_to_texture");
auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());
auto cs = shaderLib->compute("edge_filter", device.get());
cleanup.push(vs);
cleanup.push(fs);
cleanup.push(cs);
```

The device initialization is the same as in previous examples. The key difference is that `ShaderLib` loads three shader modules instead of two:

- `cube.vert.spv` -- vertex shader for the graphics pass
- `cube.frag.spv` -- fragment shader for the graphics pass
- `edge_filter.comp.spv` -- compute shader for the edge detection pass

**Reference:** [GPU API -- ShaderLibraries](../../api/gpu/ShaderLibraries.md)

### 4. Graphics pipeline layout (same as 07)

```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 1},
    gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});
graphicsLayoutDesc.debugName = "Cube pipeline layout";
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
cleanup.push(graphicsLayout);
```

The graphics pipeline layout is identical to tutorial 07 -- three descriptor sets with four bindings:

| Set | Binding | Type | Stage | Purpose |
|-----|---------|------|-------|---------|
| 0 | 0 | UniformBuffer | Vertex | Camera UBO (projectionView matrix) |
| 1 | 0 | UniformBuffer | Vertex | Model UBO (rotation matrix) |
| 2 | 0 | SampledImage | Fragment | Cube texture |
| 2 | 1 | Sampler | Fragment | Texture sampler |

**Reference:** [GPU API -- PipelineLayout](../../api/gpu/PipelineLayout.md)

### 5. Compute pipeline layout

```cpp
gpu::PipelineLayoutDescription computeLayoutDesc{};
computeLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1
});
computeLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 1, .metal = 1},
    gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1
});
computeLayoutDesc.debugName = "Edge filter pipeline layout";
auto computeLayout = device->createPipelineLayout(computeLayoutDesc);
cleanup.push(computeLayout);
```

The compute pipeline layout declares a single descriptor set (set 0) with two storage image bindings:

| Set | Binding | Type | Stage | Purpose |
|-----|---------|------|-------|---------|
| 0 | 0 | StorageImage | Compute | Input image (offscreenColor) -- read-only in the shader |
| 0 | 1 | StorageImage | Compute | Output image (computeOutput) -- writable |

The `StorageImage` resource type maps to `image2D` in GLSL, which supports random-access read/write via `imageLoad()` and `imageStore()`. Unlike `SampledImage`, storage images do not need a sampler -- the shader computes exact pixel coordinates.

**Reference:** [GPU API -- ComputePipeline](../../api/gpu/ComputePipeline.md)

### 6. Procedural texture and material resource set (same as 07)

```cpp
const std::array<std::array<uint8_t, 4>, 4> texels = {{
    {{255,   0,   0, 255}}, {{  0, 255,   0, 255}},
    {{  0,   0, 255, 255}}, {{255, 255,   0, 255}}
}};
auto texture = device->createImage({
    .size = {2, 2},
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
    .debugName = "Procedural 2x2 texture"
});
texture->uploadRGBA8(texels.data(), { 2, 2 });
device->immediateSubmit([texture](gpu::CommandBuffer* cmd) {
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
cleanup.push(texture);

auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
cleanup.push(sampler);

auto textureSet = device->createResourceSet(graphicsLayout.get(), 2, "Material resource set");
textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
textureSet->setSampler({.vulkan = 1, .metal = 1}, sampler.get());
textureSet->update();
cleanup.push(textureSet);
```

Same as tutorial 07: a 2x2 procedural texture with four colored pixels, uploaded and transitioned to `ShaderReadOnly`. The texture set is bound to descriptor set 2 and never changes.

### 7. Camera UBO and model UBO ring (same as 07)

```cpp
// Camera UBO (set 0)
float aspect = 800.0f / 600.0f;
auto projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
auto view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);
CameraUBO cameraData{};
cameraData.projectionView = projection * view;

auto cameraUbo = device->createBuffer("Camera UBO");
cameraUbo->createUniformBuffer(cameraData);
cleanup.push(cameraUbo);

auto cameraSet = device->createResourceSet(graphicsLayout.get(), 0, "Camera resource set");
cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
cameraSet->update();
cleanup.push(cameraSet);

// Model UBO ring (set 1)
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(graphicsLayout.get(), 1,
        "Model resource set ring[" + std::to_string(i) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
    set->update();
    return set;
});
```

Identical to tutorial 07: the camera UBO is persistent, the model UBO uses a `FrameResourceRing` for per-frame updates.

### 8. Cube mesh and graphics pipeline

```cpp
std::unique_ptr<bg2e::geo::MeshPU> cubeData(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));
gpu::MeshPU cube;
cube.setMeshData(*cubeData);
cube.build(device.get());

auto offscreenColorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
auto offscreenDepthFormat = gpu::PixelFormat::D32_SFLOAT;

gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat    = offscreenColorFormat;
pipelineDesc.depthFormat    = offscreenDepthFormat;
pipelineDesc.debugName      = "Cube graphics pipeline";
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);
cleanup.push(pipeline);
```

The graphics pipeline is created with the offscreen image formats instead of the surface formats. This is the critical difference from tutorial 07: the pipeline must match the format of the image it renders into, not the swapchain.

| Field | Value | Why |
|-------|-------|-----|
| `colorFormat` | R8G8B8A8_UNORM | Matches `offscreenColor` format |
| `depthFormat` | D32_SFLOAT | Matches `offscreenDepth` format |

**Reference:** [GPU API -- GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 9. Compute pipeline

```cpp
gpu::ComputePipelineDescription computePipelineDesc{};
computePipelineDesc.computeShader = cs.get();
computePipelineDesc.layout        = computeLayout.get();
computePipelineDesc.debugName     = "Edge filter compute pipeline";
auto computePipeline = device->createComputePipeline(computePipelineDesc);
cleanup.push(computePipeline);
```

The compute pipeline is created with the compute shader and its layout. The `ComputePipelineDescription` is simpler than `GraphicsPipelineDescription` because compute pipelines do not have vertex input, rasterization, or render target state.

**Reference:** [GPU API -- ComputePipeline](../../api/gpu/ComputePipeline.md)

### 10. Offscreen image creation

```cpp
auto surfaceSize = surface->size();
std::shared_ptr<gpu::Image> offscreenColor;
std::shared_ptr<gpu::Image> offscreenDepth;
std::shared_ptr<gpu::Image> computeOutput;
createOffscreenImages(device.get(), surfaceSize, offscreenColor, offscreenDepth, computeOutput, "init");

auto computeSet = createComputeResourceSet(
    device.get(), computeLayout.get(),
    offscreenColor.get(), computeOutput.get()
);
```

The three offscreen images are created at the initial surface size. The compute resource set binds `offscreenColor` as input and `computeOutput` as output.

These images are **not** pushed to `CleanupManager` because they must be recreated when the window resizes. They are cleaned up manually at shutdown.

### 11. Render loop with three passes

```cpp
auto& graphicsQueue = device->graphicsQueue();

bool running = true;
while (running)
{
    // ... event handling ...

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

    ModelUBO modelData{};
    modelData.model = glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    auto* modelUbo = modelUboRing.current();
    modelUbo->updateUniformBuffer(modelData);
    auto* modelSet = modelSetRing.current();

    cmd->begin();
```

The frame setup is identical to tutorial 07: update the model UBO with the current rotation, begin recording commands.

#### Pass 1: Render cube into offscreen color/depth

```cpp
    // --- Pass 1: Render cube into offscreen color/depth ---
    cmd->transition(offscreenColor.get(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(offscreenDepth.get(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(offscreenColor.get(), offscreenDepth.get());
    cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
    cmd->clearDepth(1.0f);

    cmd->bindPipeline(pipeline.get());
    cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
    cmd->bindResourceSet(pipeline.get(), 1, modelSet);
    cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
    cube.draw(cmd.get());

    cmd->endRendering();
```

This is the key difference from tutorial 07. Instead of `beginRendering(frame.get())` which renders into the swapchain image, `beginRendering(offscreenColor.get(), offscreenDepth.get())` renders into the offscreen images.

The `beginRendering(Image*, Image*)` overload takes two `gpu::Image*` arguments: the color attachment and the depth attachment. This is the same dynamic rendering path used for `SurfaceFrame`, but targeting regular images.

**Image layout transitions** must happen before each pass:
- `offscreenColor` transitions to `ColorAttachment` for the graphics pass
- `offscreenDepth` transitions to `DepthAttachment` for the graphics pass

After `endRendering()`, `offscreenColor` is in `ColorAttachment` layout, which the compute pass cannot read from. It must be transitioned to `General` layout first.

#### Pass 2: Compute edge detection

```cpp
    // --- Pass 2: Compute edge detection ---
    cmd->transition(offscreenColor.get(), gpu::ImageLayout::General);
    cmd->transition(computeOutput.get(), gpu::ImageLayout::General);
    cmd->beginCompute();
    cmd->bindPipeline(computePipeline.get());
    cmd->bindResourceSet(computePipeline.get(), 0, computeSet.get());
    uint32_t w = surface->size().width;
    uint32_t h = surface->size().height;
    cmd->dispatch((w + 15) / 16, (h + 15) / 16, 1);
    cmd->endCompute();
```

The compute pass reads from `offscreenColor` and writes to `computeOutput`. Both images must be in `General` layout, which is the only layout that supports both storage image reads and writes.

The `dispatch()` call computes workgroup counts to cover the entire image. The shader uses `local_size_x = 16, local_size_y = 16`, so each workgroup handles 256 pixels. The `(w + 15) / 16` formula rounds up to ensure full coverage:

| Image dimension | Workgroups | Thread coverage |
|----------------|------------|-----------------|
| 800 | 50 | 800 (exactly 50 * 16) |
| 801 | 51 | 816 (16 threads at edge are clipped by bounds check) |

**Reference:** [GPU API -- CommandBuffer dispatch()](../../api/gpu/CommandBuffer.md)

#### Pass 3: Copy result to surface

```cpp
    // --- Pass 3: Copy postprocessed image to surface ---
    cmd->transition(computeOutput.get(), gpu::ImageLayout::TransferSrc);
    cmd->transition(frame->colorImage(), gpu::ImageLayout::TransferDst);
    cmd->copyImage(computeOutput.get(), frame->colorImage());
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());
}
```

The final pass copies the edge-detected image to the swapchain image for presentation. `copyImage()` requires:
- Source image in `TransferSrc` layout
- Destination image in `TransferDst` layout

Both images must have the appropriate transfer usage flags. After the copy, the swapchain image transitions to `Present` for display.

**Reference:** [GPU API -- CommandBuffer copyImage()](../../api/gpu/CommandBuffer.md)

### 12. Resize handling

```cpp
if (event.type == SDL_WINDOWEVENT &&
    event.window.event == SDL_WINDOWEVENT_RESIZED)
{
    device->waitIdle();
    uint32_t w = event.window.data1;
    uint32_t h = event.window.data2;
    if (w == 0 || h == 0) continue;
    surface->resize({w, h});

    // Recreate offscreen images to match new surface size
    offscreenColor->cleanup();
    offscreenColor = device->createImage({
        .size = {w, h},
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::Sampled
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::Storage,
        .debugName = "Offscreen color"
    });

    offscreenDepth->cleanup();
    offscreenDepth = device->createImage({
        .size = {w, h},
        .format = gpu::PixelFormat::D32_SFLOAT,
        .usage = gpu::ImageUsage::DepthStencil
               | gpu::ImageUsage::Sampled,
        .debugName = "Offscreen depth"
    });

    computeOutput->cleanup();
    computeOutput = device->createImage({
        .size = {w, h},
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::Storage
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst
               | gpu::ImageUsage::Storage,
        .debugName = "Compute output"
    });

    // Recreate compute resource set with new images
    computeSet->cleanup();
    computeSet = createComputeResourceSet(
        device.get(), computeLayout.get(),
        offscreenColor.get(), computeOutput.get()
    );

    // Recreate the projection matrix with the new aspect ratio
    aspect = static_cast<float>(w) / static_cast<float>(h);
    projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    cameraData.projectionView = projection * view;
    cameraUbo->updateUniformBuffer(cameraData);
}
```

When the window resizes, the offscreen images must be recreated to match the new surface dimensions. The sequence is:

1. **`device->waitIdle()`** -- ensure the GPU is not using the images
2. **`surface->resize()`** -- recreate the swapchain
3. **Cleanup old images** -- call `cleanup()` on each offscreen image
4. **Create new images** -- with the new `{w, h}` size
5. **Recreate compute resource set** -- the old set referenced the old images
6. **Update camera UBO** -- the aspect ratio changed

This is why offscreen images are not pushed to `CleanupManager` -- they are recreated during the application lifetime, not just at shutdown.

### 13. Cleanup

```cpp
device->waitIdle();

modelSetRing.cleanup();
modelUboRing.cleanup();
cube.cleanup();

// Clean up offscreen images and compute set manually
computeSet->cleanup();
computeOutput->cleanup();
offscreenDepth->cleanup();
offscreenColor->cleanup();

cleanup.flush();

surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

The offscreen images and compute resource set are cleaned up manually before `cleanup.flush()` because they were not pushed to the `CleanupManager`. The rest follows the standard teardown order.

## Shader code explanation

### Vertex shader (`cube.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionView;
} camera;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

Identical to tutorial 07. The vertex shader transforms positions through the combined projection-view matrix and model matrix, then passes texture coordinates to the fragment shader.

### Fragment shader (`cube.frag.glsl`)

```glsl
#version 450

layout(set = 2, binding = 0) uniform texture2D uTex;
layout(set = 2, binding = 1) uniform sampler   uSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

Identical to tutorial 07. Samples the procedural texture using separate image and sampler from descriptor set 2.

### Compute shader (`edge_filter.comp.glsl`)

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputImage;
layout(set = 0, binding 1, rgba8) uniform image2D outputImage;
```

The compute shader declares:

- **`local_size_x = 16, local_size_y = 16`** -- each workgroup is a 16x16 grid of threads (256 threads total). This is a common size that maps well to GPU wavefronts/warps.
- **`inputImage`** (set 0, binding 0) -- `readonly image2D` with `rgba8` format. The shader can only read from this image.
- **`outputImage`** (set 0, binding 1) -- `image2D` with `rgba8` format. The shader writes to this image.

```glsl
float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}
```

The `luma()` function converts an RGB color to grayscale luminance using the standard BT.601 weights. These weights approximate human perception of brightness -- green contributes most (58.7%), red next (29.9%), blue least (11.4%).

```glsl
void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (pos.x >= size.x || pos.y >= size.y) return;
```

`gl_GlobalInvocationID` is the global thread index: `(workgroup_id * local_size) + local_invocation_id`. Each thread processes exactly one pixel. The bounds check ensures threads at the image edge (where workgroup size exceeds image dimensions) do not read out-of-bounds pixels.

```glsl
    // Sample 3x3 neighborhood luminances
    float tl = luma(imageLoad(inputImage, pos + ivec2(-1, -1)).rgb);
    float t  = luma(imageLoad(inputImage, pos + ivec2( 0, -1)).rgb);
    float tr = luma(imageLoad(inputImage, pos + ivec2( 1, -1)).rgb);
    float l  = luma(imageLoad(inputImage, pos + ivec2(-1,  0)).rgb);
    float r  = luma(imageLoad(inputImage, pos + ivec2( 1,  0)).rgb);
    float bl = luma(imageLoad(inputImage, pos + ivec2(-1,  1)).rgb);
    float b  = luma(imageLoad(inputImage, pos + ivec2( 0,  1)).rgb);
    float br = luma(imageLoad(inputImage, pos + ivec2( 1,  1)).rgb);
```

`imageLoad()` reads a pixel from a storage image at arbitrary integer coordinates. The shader loads the 8 neighboring pixels (excluding center) of the current pixel and converts each to luminance. The naming convention is compass directions: `tl` = top-left, `t` = top, `tr` = top-right, `l` = left, `r` = right, `bl` = bottom-left, `b` = bottom, `br` = bottom-right.

```glsl
    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0, 1.0);

    imageStore(outputImage, pos, vec4(vec3(edge), 1.0));
}
```

The Sobel operator detects edges by computing the image gradient:

**Horizontal gradient (Gx):**
```
-1  0  1       tl  t  tr
-2  0  2   =   l   .  r
-1  0  1       bl  b  br
```

**Vertical gradient (Gy):**
```
-1 -2 -1       tl  t  tr
 0  0  0   =   l   .  r
 1  2  1       bl  b  br
```

The magnitude `sqrt(gx^2 + gy^2)` gives the edge strength. The result is clamped to [0, 1] and written as a grayscale image via `imageStore()`. Strong edges (cube silhouette) appear white, flat areas appear black.

## Key concepts

### Image layout transitions

Every image operation requires a specific layout. This example uses five different layouts across the three passes:

| Layout | Used by | Pass |
|--------|---------|------|
| `ColorAttachment` | Graphics fragment shader output | Pass 1 |
| `DepthAttachment` | Depth testing | Pass 1 |
| `General` | Compute shader storage image read/write | Pass 2 |
| `TransferSrc` | Source for `copyImage()` | Pass 3 |
| `TransferDst` | Destination for `copyImage()` | Pass 3 |
| `Present` | Swapchain presentation | Pass 3 |

Failing to transition images to the correct layout before use causes undefined behavior. The GPU abstraction layer records transitions via `CommandBuffer::transition()`.

**Reference:** [GPU API -- CommandBuffer transition()](../../api/gpu/CommandBuffer.md)

### Offscreen vs. surface rendering

The `beginRendering()` method has two overloads:

| Overload | Target | Use case |
|----------|--------|----------|
| `beginRendering(SurfaceFrame*)` | Swapchain image | Direct-to-screen rendering |
| `beginRendering(Image*, Image*)` | Offscreen images | Post-processing, shadow maps, reflections |

The offscreen overload takes explicit color and depth images. The graphics pipeline must be created with formats matching these images, not the surface.

### Storage images in compute shaders

Compute shaders access images via `image2D` descriptors, not `texture2D`:

| Feature | `texture2D` (sampled) | `image2D` (storage) |
|---------|----------------------|---------------------|
| Access | `texture(sampler2D(tex, samp), uv)` | `imageLoad(img, ivec2)` / `imageStore(img, ivec2, value)` |
| Coordinates | Normalized [0,1] UV | Integer pixel coordinates |
| Filtering | Bilinear, nearest, etc. | None (exact pixel) |
| Descriptor type | `SampledImage` + `Sampler` | `StorageImage` |
| Layout | `ShaderReadOnly` | `General` |

Storage images are more flexible (random read/write) but do not benefit from hardware texture filtering. They are the standard choice for compute post-processing.

### Workgroup dispatch sizing

The compute shader uses 16x16 workgroups. To cover an image of arbitrary size, the dispatch counts are computed as:

```cpp
cmd->dispatch((w + 15) / 16, (h + 15) / 16, 1);
```

This rounds up using integer division. The shader's bounds check (`if (pos.x >= size.x || pos.y >= size.y) return;`) handles the extra threads at image edges.

### Lifecycle of offscreen resources

Offscreen images and their associated resource sets are managed separately from `CleanupManager`:

| Resource | CleanupManager? | Reason |
|----------|----------------|--------|
| Pipeline, layout, shaders | Yes | Created once, never recreated |
| Camera UBO, texture set | Yes | Created once, never recreated |
| Model UBO ring, model set ring | No (manual) | Per-frame resources with cross-references |
| Offscreen images, compute set | No (manual) | Recreated on window resize |

The manual cleanup pattern is: call `cleanup()` on each object, then on old shared_ptr release. This gives explicit control over destruction order.

## Building and running

Build the example with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_render_to_texture
```

Then run the binary from the `bin/` directory. On macOS you will be prompted to choose a backend. You should see a window with an edge-detected rotating cube -- the cube silhouette appears as white lines on a black background.

## Next steps

- **[09_cubemap](09_cubemap.md)** -- Render into a cubemap for environment mapping.
- **[GPU API -- Image](../../api/gpu/Image.md)** -- Full reference for image creation and usage flags.
- **[GPU API -- CommandBuffer](../../api/gpu/CommandBuffer.md)** -- Full reference for transitions, rendering, compute dispatch, and image copy.
- **[GPU API -- ComputePipeline](../../api/gpu/ComputePipeline.md)** -- Full reference for compute pipeline creation.
- **[GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)** -- Full reference for storage image binding.
