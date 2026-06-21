# Tutorial 05: Simple Triangle with Pipelines and Shaders

This tutorial walks through the `05_simple_triangle` example: rendering a textured pentagon on top of a compute-generated gradient background. Despite its name, this example does much more than draw a triangle -- it introduces shaders, pipeline layouts, resource sets, push constants, textures, samplers, meshes, and compute pipelines. It is the most important foundational example in the GPU abstraction layer.

**Source:** `examples/gpu/05_simple_triangle/src/main.cpp`

## What you will learn

- How to create and load shader modules (Vulkan SPIR-V and Metal metallib)
- How to configure pipeline layouts with push constants and resource bindings
- How to create and upload a procedural texture
- How to create and bind a sampler
- How to create resource sets (descriptor sets) for texture/sampler binding
- How to build a mesh with vertex and index data
- How to create graphics and compute pipelines
- How to dispatch a compute shader that writes directly to the swapchain image
- How to render geometry with push constants and texture sampling
- The full render loop with compute + graphics passes

## Prerequisites

- Completed [04_clear_loop](04_clear_loop.md) -- you should understand the render loop, frame lifecycle, and image transitions
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding the example

This example renders two things per frame:

1. **A gradient background** -- a compute shader writes a UV-gradient directly into the swapchain color image. This is the first time we write to the screen without using `clearColor()`.

2. **A textured pentagon** -- a graphics pipeline renders a procedural pentagon (5 triangles fanning from the center) with a 2x2 procedural texture, tinted by an animated color via push constants.

The combination demonstrates how compute and graphics passes share the same color image within a single command buffer.

## Step-by-step code explanation

### 1. Headers and includes

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
```

| Header | Purpose |
|--------|---------|
| `<bg2e.hpp>` | Engine umbrella header |
| `<bg2e/gpu/all.hpp>` | GPU abstraction layer |
| `<bg2e/app/SDLUtils.hpp>` | SDL helper utilities |
| `<array>`, `<vector>` | STL containers for texel data and resource set ring |
| `<cmath>` | `std::sin()` for animated push constant color |
| `<memory>` | `std::shared_ptr`, `std::unique_ptr` for GPU resource ownership |

### 2. Push constants structure

```cpp
struct PushConstants {
    float color[4];
};
```

This struct matches the push constant block in the fragment shader. It carries an animated RGBA color used to tint the texture. The struct size (16 bytes) must exactly match the `sizeof` passed to the pipeline layout.

**Reference:** [PipelineLayout API -- push constants](../../api/gpu/PipelineLayout.md), [CommandBuffer API -- pushConstants()](../../api/gpu/CommandBuffer.md)

### 3. Device initialization (steps 1-7)

The first seven steps are identical to [04_clear_loop](04_clear_loop.md). They create the backend, window, instance, surface, physical device, and logical device. We will not repeat them here -- see the previous tutorial for details.

**Reference:** [Factory API](../../api/gpu/Factory.md), [Instance API](../../api/gpu/Instance.md), [Device API](../../api/gpu/Device.md)

### 4. Shader module creation (step 8, part 1)

```cpp
auto shaderBasePath = base::PlatformTools::shaderPath();
std::string targetName = "gpu_simple_triangle";

std::shared_ptr<gpu::ShaderModule> vs;
std::shared_ptr<gpu::ShaderModule> fs;
std::shared_ptr<gpu::ShaderModule> cs;

if (backendType == gpu::BackendType::Vulkan)
{
    auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
    auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
    auto csPath = (shaderBasePath / targetName / "gradient.comp.spv").string();
    vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
    fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
    cs = device->createShaderModule({ csPath, "main", gpu::ShaderStage::Compute,  "Gradient compute shader" });
}
else
{
    auto vsPath = (shaderBasePath / targetName / "triangle.vert.metallib").string();
    auto fsPath = (shaderBasePath / targetName / "triangle.frag.metallib").string();
    auto csPath = (shaderBasePath / targetName / "gradient.comp.metallib").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
    cs = device->createShaderModule({ csPath, "compMain", gpu::ShaderStage::Compute,  "Gradient compute shader" });
}
```

Key points:

- **Vulkan** loads `.spv` (SPIR-V) files with entry point `"main"`.
- **Metal** loads `.metallib` files with named entry points (`"vertMain"`, `"fragMain"`, `"compMain"`).
- `ShaderModuleDescription` carries four fields: file path, entry point, stage, and debug name.
- Three shader stages are needed: vertex, fragment, and compute.

**Reference:** [ShaderModule API](../../api/gpu/ShaderModule.md)

### 5. Graphics pipeline layout (step 8, part 2)

```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
graphicsLayoutDesc.resourceBindings.push_back({ 0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1 });
graphicsLayoutDesc.resourceBindings.push_back({ 0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler,      gpu::ShaderStage::Fragment, 1 });
graphicsLayoutDesc.debugName = "Graphics pipeline layout";
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
```

This layout defines two things:

**Push constants** -- a 16-byte block (one `vec4`) available in the fragment stage at offset 0. The GPU pushes this data directly into shader registers without any buffer allocation.

**Resource bindings** -- two bindings in descriptor set 0:

| Set | Vulkan binding | Metal index | Type | Stage | GLSL equivalent |
|-----|---------------|-------------|------|-------|-----------------|
| 0 | 0 | 0 | SampledImage | Fragment | `layout(set=0, binding=0) uniform texture2D uTex` |
| 0 | 1 | 1 | Sampler | Fragment | `layout(set=0, binding=1) uniform sampler uSampler` |

The `ShaderBinding` struct carries both Vulkan and Metal indices. Vulkan uses `binding.vulkan` as the descriptor binding index within the set. Metal uses `binding.metal` as the `[[texture(N)]]` or `[[sampler(N)]]` argument index.

**Reference:** [PipelineLayout API](../../api/gpu/PipelineLayout.md)

### 6. Procedural texture creation (step 8, part 3)

```cpp
const std::array<std::array<uint8_t,4>, 4> texels = {{
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
device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
{
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
```

This creates a tiny 2x2 texture with four solid colors:

```
Red (255,0,0)      Green (0,255,0)
Blue (0,0,255)     Yellow (255,255,0)
```

The process is:

1. **Create the image** with `Sampled` (for shader reads) and `TransferDst` (for staging upload) usage flags.
2. **Upload texel data** via `uploadRGBA8()`. This internally allocates a staging buffer, copies the data, and submits a transfer command.
3. **Transition to ShaderReadOnly** via `immediateSubmit()`. The image starts in an undefined layout; the transition makes it safe to sample in the fragment shader.

`immediateSubmit()` records a one-shot command buffer, submits it synchronously, and blocks until the GPU finishes. This is the simplest way to perform a one-time operation.

**Reference:** [Image API](../../api/gpu/Image.md), [Device API -- immediateSubmit()](../../api/gpu/Device.md)

### 7. Sampler creation (step 8, part 4)

```cpp
auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
```

Creates a sampler with default settings: linear min/mag filters and repeat address mode. The sampler is a separate object from the texture -- both must be bound to the resource set.

**Reference:** [Image API -- SamplerDescription](../../api/gpu/Image.md)

### 8. Resource set for texture + sampler (step 8, part 5)

```cpp
auto textureSet = device->createResourceSet(graphicsLayout.get(), 0, "Static texture set");
textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
textureSet->setSampler({.vulkan = 1, .metal = 1}, sampler.get());
textureSet->update();
```

A resource set (Vulkan descriptor set) groups all resources bound to a single descriptor set index. Here:

- `createResourceSet(layout, 0)` creates a set bound to set index 0 of the graphics layout.
- `setSampledImage()` binds the texture to binding 0 (Vulkan) / texture(0) (Metal).
- `setSampler()` binds the sampler to binding 1 (Vulkan) / sampler(1) (Metal).
- `update()` flushes the assignments to the backend (calls `vkUpdateDescriptorSets` on Vulkan, or caches the assignments for Metal encoder replay).

This resource set is created once and reused every frame because the texture and sampler never change.

**Reference:** [ResourceSet API](../../api/gpu/ResourceSet.md)

### 9. Compute pipeline layout (step 8, part 6)

```cpp
gpu::PipelineLayoutDescription computeLayoutDesc{};
computeLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1
});
computeLayoutDesc.debugName = "Compute pipeline layout";
auto computeLayout = device->createPipelineLayout(computeLayoutDesc);
```

The compute layout has a single binding: a storage image at set 0, binding 0. This is the image the compute shader will write to. In GLSL, this corresponds to:

```glsl
layout(set = 0, binding = 0, rgba8) uniform image2D outImage;
```

**Reference:** [PipelineLayout API](../../api/gpu/PipelineLayout.md)

### 10. Mesh creation (step 8, part 7)

```cpp
bg2e::geo::MeshPU meshData;
meshData.vertices = {
    // perimeter (angle = -90 + i*72)
    { { 0.0000f, -0.5000f, 0.0f }, { 0.500f, 0.000f } },   // 0 - top
    { { 0.4755f, -0.1545f, 0.0f }, { 0.976f, 0.345f } },   // 1 - upper-right
    { { 0.2939f,  0.4045f, 0.0f }, { 0.794f, 0.905f } },   // 2 - lower-right
    { {-0.2939f,  0.4045f, 0.0f }, { 0.206f, 0.905f } },   // 3 - lower-left
    { {-0.4755f, -0.1545f, 0.0f }, { 0.024f, 0.345f } },   // 4 - upper-left
    // centre
    { { 0.0000f,  0.0000f, 0.0f }, { 0.500f, 0.500f } },   // 5
};
meshData.indices = {
    5, 0, 1,
    5, 1, 2,
    5, 2, 3,
    5, 3, 4,
    5, 4, 0,
};
meshData.submeshes = { { 0, 15 } };

gpu::MeshPU mesh;
mesh.setMeshData(meshData);
mesh.build(device.get());
```

The mesh is a regular pentagon built from 6 vertices (5 on the perimeter + 1 at the center) and 5 triangles. Each vertex has a position (`vec3`) and a texture coordinate (`vec2`), matching the `MeshPU` vertex layout.

The `MeshPU` type maps to the vertex attribute locations:

| Location | Attribute | Format |
|----------|-----------|--------|
| 0 | Position | `R32G32B32_SFLOAT` |
| 1 | TexCoord0 | `R32G32_SFLOAT` |

`mesh.build(device)` allocates GPU vertex and index buffers and uploads the data via staging.

**Reference:** [Mesh API](../../api/gpu/Mesh.md)

### 11. Graphics pipeline creation (step 8, part 8)

```cpp
auto colorFormat = surface->colorFormat();
auto depthFormat = surface->depthFormat();

gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat    = colorFormat;
pipelineDesc.depthFormat    = depthFormat;
pipelineDesc.debugName      = "Main graphics pipeline";
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());

auto pipeline = device->createGraphicsPipeline(pipelineDesc);
```

The graphics pipeline description ties together:

| Field | Value | Meaning |
|-------|-------|---------|
| `vertexShader` | `vs` | Vertex shader module |
| `fragmentShader` | `fs` | Fragment shader module |
| `layout` | `graphicsLayout` | Push constants + resource bindings |
| `topology` | `TriangleList` | Every 3 indices form a triangle |
| `colorFormat` | Surface color format | Must match the swapchain image format |
| `depthFormat` | Surface depth format | Must match the depth image format |
| `vertexBufferDescriptions` | `MeshPU::vertexBufferDescription()` | Describes the vertex attribute layout |

The `addVertexBufferDescription()` call is critical. It tells the pipeline how to interpret the vertex buffer. Without it, the pipeline assumes procedurally-generated vertices (no vertex buffer input).

**Reference:** [GraphicsPipeline API](../../api/gpu/GraphicsPipeline.md)

### 12. Compute pipeline creation (step 8, part 9)

```cpp
gpu::ComputePipelineDescription computePipelineDesc{};
computePipelineDesc.computeShader = cs.get();
computePipelineDesc.layout        = computeLayout.get();
computePipelineDesc.debugName     = "Gradient compute pipeline";
auto computePipeline = device->createComputePipeline(computePipelineDesc);
```

Simpler than the graphics pipeline -- just a compute shader and a layout. The compute pipeline is used to dispatch the gradient computation.

**Reference:** [ComputePipeline API](../../api/gpu/ComputePipeline.md)

### 13. Compute resource set ring (step 8, part 10)

```cpp
auto imageCount = surface->imageCount();
std::vector<std::shared_ptr<gpu::ResourceSet>> resourceSets;
resourceSets.reserve(imageCount);
for (uint32_t i = 0; i < imageCount; ++i)
{
    resourceSets.push_back(device->createResourceSet(computeLayout.get(), 0,
        "Compute resource set ring[" + std::to_string(i) + "]"));
}
uint32_t ringIndex = 0;
```

The compute shader writes to the swapchain color image. Since there are multiple swapchain images (double or triple buffering), each needs its own resource set. The resource sets are pre-created but their storage image binding is updated each frame with the current frame's color image.

The `ringIndex` tracks which resource set corresponds to the current frame.

### 14. Render loop -- event handling and push constants

```cpp
auto& graphicsQueue = device->graphicsQueue();

bool running = true;
while (running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            surface->resize({ static_cast<uint32_t>(event.window.data1),
                              static_cast<uint32_t>(event.window.data2) });
        }
    }

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

    PushConstants push{};
    push.color[0] = 0.5f + 0.5f * std::sin(t * 1.3f);
    push.color[1] = 0.5f + 0.5f * std::sin(t * 1.7f + 1.0f);
    push.color[2] = 0.5f + 0.5f * std::sin(t * 2.1f + 2.0f);
    push.color[3] = 1.0f;
```

The push constants are computed each frame with three sine waves at different frequencies. This produces a smoothly varying RGB tint that modulates the texture color in the fragment shader.

### 15. Render loop -- compute pass

```cpp
    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    cmd->begin();

    // Transition color image to General for compute write
    cmd->transition(frame->colorImage(), gpu::ImageLayout::General);

    // Compute dispatch: write gradient into the color image
    cmd->beginCompute();
    cmd->bindPipeline(computePipeline.get());

    auto* rs = resourceSets[ringIndex].get();
    rs->setStorageImage({.vulkan = 0, .metal = 0}, frame->colorImage());
    rs->update();
    cmd->bindResourceSet(computePipeline.get(), 0, rs);

    uint32_t gx = (frame->colorImage()->width()  + 15) / 16;
    uint32_t gy = (frame->colorImage()->height() + 15) / 16;
    cmd->dispatch(gx, gy, 1);

    cmd->endCompute();
```

This is the first compute pass in the tutorial series. The steps are:

1. **Transition to `General`** -- compute shaders require the `General` image layout for storage image read/write.
2. **Begin compute pass** -- starts a compute encoder (Metal) or simply marks the begin of compute commands (Vulkan).
3. **Bind the compute pipeline** -- selects the gradient compute shader.
4. **Update and bind the resource set** -- binds the swapchain color image as a storage image. This must be done per-frame because the color image changes each frame.
5. **Dispatch** -- launches work groups. The compute shader uses 16x16 local size, so we divide the image dimensions by 16 (rounding up with `+15`).
6. **End compute pass** -- finalizes the compute encoder.

The gradient compute shader writes a UV-based color (red-green gradient with blue=0.5) directly into the swapchain image, replacing the need for `clearColor()`.

**Reference:** [CommandBuffer API -- dispatch()](../../api/gpu/CommandBuffer.md), [ResourceSet API](../../api/gpu/ResourceSet.md)

### 16. Render loop -- graphics pass

```cpp
    // Graphics rendering (gradient is the background, no clearColor)
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearDepth(1.0f);
    cmd->bindPipeline(pipeline.get());
    cmd->bindResourceSet(pipeline.get(), 0, textureSet.get());
    cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
    mesh.draw(cmd.get());
    cmd->endRendering();
```

After the compute pass writes the gradient, the graphics pass renders the pentagon on top:

1. **Transition color image to `ColorAttachment`** -- the compute pass left it in `General`; we need `ColorAttachment` for rendering.
2. **Transition depth image to `DepthAttachment`** -- required for depth testing.
3. **Begin rendering** -- starts a dynamic rendering pass using the frame's color and depth images.
4. **Clear depth to 1.0** -- no `clearColor()` because the gradient from the compute pass is the background.
5. **Bind graphics pipeline** -- selects the vertex/fragment shaders and their configuration.
6. **Bind texture resource set** -- makes the texture and sampler available to the fragment shader.
7. **Push constants** -- uploads the animated RGBA color to the fragment shader's push constant block.
8. **Draw the mesh** -- `mesh.draw(cmd)` internally binds the vertex buffer, binds the index buffer, and issues `drawIndexed` for each submesh.
9. **End rendering** -- finalizes the rendering pass.

**Reference:** [CommandBuffer API -- bindResourceSet(), pushConstants()](../../api/gpu/CommandBuffer.md), [Mesh API -- draw()](../../api/gpu/Mesh.md)

### 17. Render loop -- presentation and frame completion

```cpp
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());

    ringIndex = (ringIndex + 1) % imageCount;
}
```

The final steps are identical to the clear loop: transition to `Present`, record presentation, end the command buffer, submit, and end the frame. The `ringIndex` advances to the next swapchain image for the next frame.

### 18. Cleanup

```cpp
device->waitIdle();
mesh.cleanup();
for (auto& rs : resourceSets)
{
    rs->cleanup();
}
computePipeline->cleanup();
pipeline->cleanup();
textureSet->cleanup();
sampler.reset();
texture.reset();
computeLayout->cleanup();
graphicsLayout->cleanup();
cs->cleanup();
vs->cleanup();
fs->cleanup();
surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

Cleanup happens in reverse creation order:

1. Wait for GPU idle
2. Free mesh buffers
3. Free compute resource set ring
4. Free pipelines
5. Free texture resource set
6. Free sampler and texture (via `reset()` on `shared_ptr`)
7. Free pipeline layouts
8. Free shader modules
9. Free surface, device, instance
10. Destroy SDL window and quit

**Reference:** [DeviceResource and resource management](../../api/gpu/DeviceResource_and_resource_management.md)

## Shader code explanation

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

This is a pass-through vertex shader:

- **Inputs:** `inPosition` at location 0 (matches `MeshPU` position attribute) and `inTexCoord` at location 1 (matches `MeshPU` texcoord attribute).
- **Outputs:** `fragUV` passed to the fragment shader via rasterization.
- **`gl_Position`:** sets the clip-space position directly from the vertex position (no transformation since the geometry is already in normalized device coordinates).

The pentagon vertices are already in the range [-0.5, 0.5], which maps directly to the visible area of the screen (NDC range [-1, 1]).

### Fragment shader (`triangle.frag.glsl`)

```glsl
#version 450

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler   uSampler;

layout(push_constant) uniform Push {
    vec4 color;
} pc;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 tex = texture(sampler2D(uTex, uSampler), fragUV);
    outColor = vec4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}
```

Key concepts:

- **Separate texture and sampler** -- Vulkan (and this API) uses separate `texture2D` and `sampler` objects instead of a combined `sampler2D`. The `sampler2D(uTex, uSampler)` constructor combines them at the call site.
- **Push constants** -- `pc.color` receives the animated RGBA value pushed from the CPU each frame.
- **Color modulation** -- the texture color is multiplied by the push constant color (RGB multiplied, alpha multiplied). This tints the texture with the animated color.
- **Descriptor set/binding layout** -- `set=0, binding=0` and `set=0, binding=1` match the resource bindings declared in the pipeline layout.

### Compute shader (`gradient.comp.glsl`)

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform image2D outImage;

void main()
{
    ivec2 size = imageSize(outImage);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= size.x || pos.y >= size.y) return;
    vec2 uv = vec2(float(pos.x) / float(size.x), float(pos.y) / float(size.y));
    imageStore(outImage, pos, vec4(uv.x, uv.y, 0.5, 1.0));
}
```

Key concepts:

- **Local size** -- `layout(local_size_x = 16, local_size_y = 16) in;` defines a 16x16 work group. The dispatch size on the CPU must cover the entire image with these work groups.
- **`image2D`** -- a storage image that can be written to with `imageStore()`. This is different from a `texture2D` which is read-only.
- **`gl_GlobalInvocationID`** -- the global pixel coordinate. Each invocation handles one pixel.
- **Bounds check** -- the `if` guard prevents writing outside the image when the dimensions are not exact multiples of 16.
- **UV gradient** -- the pixel coordinate is normalized to [0, 1] to produce a red-green gradient. Blue is fixed at 0.5.

## Key concepts

### Push constants vs. uniform buffers

Push constants are the fastest way to send small amounts of data from the CPU to shaders. They are limited to 128 bytes on most GPUs (this example uses 16 bytes). Unlike uniform buffers, they do not require any buffer allocation -- the data is pushed directly into shader registers.

Use push constants for per-draw or per-frame data that changes frequently (animated colors, transformation matrices for simple objects). Use uniform buffers for larger or less frequently updated data.

**Reference:** [CommandBuffer API -- pushConstants()](../../api/gpu/CommandBuffer.md)

### Separate texture and sampler

The `bg2e::gpu` API uses separate texture and sampler objects, which is the native Vulkan model. In GLSL, this is expressed as:

```glsl
layout(set=0, binding=0) uniform texture2D uTex;
layout(set=0, binding=1) uniform sampler   uSampler;
// ...
texture(sampler2D(uTex, uSampler), uv);
```

This allows you to combine different textures with different samplers efficiently without duplicating either object.

**Reference:** [ResourceSet API -- setSampledImage(), setSampler()](../../api/gpu/ResourceSet.md)

### Compute shaders writing to swapchain images

This example demonstrates a powerful pattern: using a compute shader to generate the background directly in the swapchain image. The key steps are:

1. Transition the color image to `General` layout
2. Bind the image as a `StorageImage` in a resource set
3. Dispatch the compute shader
4. Transition to `ColorAttachment` for the subsequent graphics pass

This avoids the need for `clearColor()` and gives you full programmatic control over the background.

### Resource set per swapchain image

When a resource set references a swapchain image (which changes each frame), you need one resource set per swapchain image. This example pre-creates them in a ring and updates the storage image binding each frame:

```cpp
auto* rs = resourceSets[ringIndex].get();
rs->setStorageImage({.vulkan = 0, .metal = 0}, frame->colorImage());
rs->update();
```

For resources that do not change (like the texture + sampler), a single persistent resource set suffices.

### Image layout transitions

Every image operation requires the image to be in a specific layout. This example uses three layouts for the color image:

| Layout | Used by |
|--------|---------|
| `General` | Compute shader storage image write |
| `ColorAttachment` | Graphics fragment shader output |
| `Present` | Display controller read |

The transitions must happen in the correct order within the command buffer. The GPU abstraction layer handles the underlying pipeline barriers.

**Reference:** [CommandBuffer API -- transition()](../../api/gpu/CommandBuffer.md)

### The MeshPU vertex layout

`MeshPU` is a vertex type with position and one texture coordinate:

```cpp
struct VertexPU {
    float position[3];   // location 0
    float texCoord0[2];  // location 1
};
```

The matching GLSL inputs are:

```glsl
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
```

When creating a pipeline for this mesh type, always call:

```cpp
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
```

**Reference:** [Mesh API -- vertexBufferDescription()](../../api/gpu/Mesh.md)

## Complete render loop flow

Here is the full sequence of GPU operations each frame:

```
surface->beginFrame()                    // acquire swapchain image
  cmd = queue.createCommandBuffer()
  cmd->begin()

  // --- Compute pass ---
  cmd->transition(color, General)
  cmd->beginCompute()
  cmd->bindPipeline(computePipeline)
  rs->setStorageImage(colorImage)        // update per frame
  rs->update()
  cmd->bindResourceSet(compute, 0, rs)
  cmd->dispatch(ceil(w/16), ceil(h/16), 1)
  cmd->endCompute()

  // --- Graphics pass ---
  cmd->transition(color, ColorAttachment)
  cmd->transition(depth, DepthAttachment)
  cmd->beginRendering(frame)
  cmd->clearDepth(1.0)
  cmd->bindPipeline(graphicsPipeline)
  cmd->bindResourceSet(graphics, 0, textureSet)
  cmd->pushConstants(Fragment, 0, 16, &push)
  mesh.draw(cmd)
  cmd->endRendering()

  // --- Presentation ---
  cmd->transition(color, Present)
  surface->present(cmd)
  cmd->end()
  queue.submit(cmd)
  surface->endFrame(frame)
```

## Building and running

Build the example with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_simple_triangle
```

Then run the binary from the `bin/` directory. You should see a window with a red-green gradient background and a textured pentagon that shifts through colors over time. Close the window to exit.

## Next steps

- **[GPU API Quick Start -- Recipe 7](../../api/gpu/quick_start.md)** -- Uniform buffers and multi-set rendering with `FrameResourceRing`.
- **[GPU API Quick Start -- Recipe 8](../../api/gpu/quick_start.md)** -- Render to texture with compute post-processing.
- **[ResourceSet API](../../api/gpu/ResourceSet.md)** -- Full reference for descriptor set binding.
- **[CommandBuffer API](../../api/gpu/CommandBuffer.md)** -- Complete list of recording methods.
