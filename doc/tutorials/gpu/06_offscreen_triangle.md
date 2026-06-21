# Tutorial 06: Offscreen Triangle with Pixel Readback

This tutorial walks through the `06_offscreen_triangle` example: rendering a triangle without a window and reading back the pixels to save an image. This is the foundational example for headless GPU rendering and demonstrates how to use the GPU abstraction layer for offscreen workloads.

**Source:** `examples/gpu/06_offscreen_triangle/src/main.cpp`

## What you will learn

- How to initialize the GPU without SDL or a window for offscreen rendering
- How to create an offscreen surface with fixed dimensions
- How to create shader modules for Vulkan SPIR-V and Metal metallib
- How to configure pipeline layouts with push constants
- How to create graphics and compute pipelines
- How to perform a single render iteration (no event loop)
- How to dispatch a compute shader offscreen
- How to read back pixels from the GPU and save them to disk
- The correct cleanup order for offscreen resources

## Prerequisites

- Completed [03_offscreen_device](03_offscreen_device.md) -- you should understand offscreen initialization
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding offscreen rendering

Offscreen rendering allows you to use the GPU without creating an OS window. This example extends the offscreen device setup from tutorial 03 by adding actual rendering: a triangle drawn with a graphics pipeline, a compute dispatch, and pixel readback to save the result as an image file.

The key differences from windowed rendering:

| Aspect | Windowed | Offscreen |
|--------|----------|-----------|
| **Instance creation** | `instance->create(window)` | `instance->create()` |
| **Surface creation** | `backend->createWindowSurface(instance)` | `backend->createOffscreenSurface(instance, size)` |
| **SDL dependency** | Required | Not required |
| **Event loop** | Continuous `while(running)` | Single iteration |
| **Presentation** | To screen via swapchain | To memory via `readPixelsRGBA8()` |
| **Image count** | 2-3 (swapchain) | 1 (single render target) |

**Reference:** [OffscreenSurface API](../../api/gpu/OffscreenSurface.md), [GPU API Index -- Offscreen rendering](../../api/gpu/index.md#offscreen-rendering)

## Step-by-step code explanation

### 1. Headers and includes

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <bg2e/db/image.hpp>
#include <filesystem>
#include <vector>
#include <iostream>
```

| Header | Purpose |
|--------|---------|
| `<bg2e/gpu/all.hpp>` | GPU abstraction layer (Factory, Backend, Device, etc.) |
| `<bg2e/base/all.hpp>` | Platform utilities (`base::PlatformTools`) |
| `<bg2e/db/image.hpp>` | Image saving (`db::saveImage()`) |
| `<filesystem>` | Path manipulation for output file |
| `<vector>` | Pixel readback buffer |
| `<iostream>` | Console output |

Note that this example does **not** include `<bg2e/app/SDLUtils.hpp>` because no SDL window is created.

### 2. Push constants structure

```cpp
struct PushConstants {
    float color[4];
};
```

This struct matches the push constant block in the fragment shader. It carries an RGBA color used to tint the triangle. The struct size (16 bytes) must exactly match the `sizeof` passed to the pipeline layout.

**Reference:** [PipelineLayout API -- push constants](../../api/gpu/PipelineLayout.md), [CommandBuffer API -- pushConstants()](../../api/gpu/CommandBuffer.md)

### 3. Backend selection with user input

```cpp
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
{
    std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
    int choice = 0;
    std::cin >> choice;
    backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
}
```

The engine supports two backends:

| Backend | `BackendType` value | Available on |
|---------|---------------------|--------------|
| Vulkan | `gpu::BackendType::Vulkan` | All platforms |
| Metal | `gpu::BackendType::Metal` | macOS only |

On Linux and Windows, Vulkan is the only option. On macOS, both Vulkan (via MoltenVK) and Metal are available, so this example lets the user choose at runtime.

**Reference:** [BackendType API](../../api/gpu/BackendType.md)

### 4. Initializing the backend factory

```cpp
gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
```

`Factory::init()` creates the concrete `Backend` implementation and stores it as a static singleton. `Factory::backend()` retrieves that singleton.

**Reference:** [Factory API](../../api/gpu/Factory.md)

### 5. Creating GPU instance without window

```cpp
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create();
```

Three operations happen here:

1. **`sharedInstance()`** -- retrieves the singleton `Instance` object from the backend.
2. **`enableDebugMode(true)`** -- enables validation layers (Vulkan) or debug instrumentation (Metal).
3. **`create()`** -- creates the instance in headless/offscreen mode with **no window parameter**.

**Reference:** [Instance API](../../api/gpu/Instance.md)

### 6. Creating offscreen surface with fixed dimensions

```cpp
const gpu::Size2D size{ 800, 600 };
std::shared_ptr<gpu::Surface> surface = backend->createOffscreenSurface(instance, size);
```

This creates a rendering target with fixed dimensions (800x600 pixels). Unlike `WindowSurface`, which automatically negotiates formats with the display system, `OffscreenSurface` uses sensible defaults:

| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| `colorFormat` | `R8G8B8A8_UNORM` | 32-bit RGBA color |
| `depthFormat` | `D32_SFLOAT` | 32-bit depth buffer |

**Reference:** [OffscreenSurface API](../../api/gpu/OffscreenSurface.md)

### 7. Physical device selection

```cpp
auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);
```

`PhysicalDevice::choose()` enumerates all available GPUs, scores them using a weighted algorithm, and selects the one with the highest score. The scoring algorithm prefers discrete GPUs with ray tracing support.

**Reference:** [PhysicalDevice API](../../api/gpu/PhysicalDevice.md)

### 8. Logical device creation

```cpp
auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());
```

The logical `Device` is the primary interface for all GPU operations. It requires the instance, physical device, and surface to find compatible queue families.

**Reference:** [Device API](../../api/gpu/Device.md)

### 9. Shader module creation

```cpp
auto shaderBasePath = base::PlatformTools::shaderPath();
std::string targetName = "gpu_offscreen_triangle";

std::shared_ptr<gpu::ShaderModule> vs;
std::shared_ptr<gpu::ShaderModule> fs;
std::shared_ptr<gpu::ShaderModule> cs;

if (backendType == gpu::BackendType::Vulkan)
{
    auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
    auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
    auto csPath = (shaderBasePath / targetName / "noop.comp.spv").string();
    vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
    fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
    cs = device->createShaderModule({ csPath, "main", gpu::ShaderStage::Compute,  "Noop compute shader" });
}
else
{
    auto vsPath = (shaderBasePath / targetName / "triangle.vert.metallib").string();
    auto fsPath = (shaderBasePath / targetName / "triangle.frag.metallib").string();
    auto csPath = (shaderBasePath / targetName / "noop.comp.metallib").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
    cs = device->createShaderModule({ csPath, "compMain", gpu::ShaderStage::Compute,  "Noop compute shader" });
}
```

Key points:

- **Vulkan** loads `.spv` (SPIR-V) files with entry point `"main"`.
- **Metal** loads `.metallib` files with named entry points (`"vertMain"`, `"fragMain"`, `"compMain"`).
- `ShaderModuleDescription` carries four fields: file path, entry point, stage, and debug name.
- Three shader stages are needed: vertex, fragment, and compute.

**Reference:** [ShaderModule API](../../api/gpu/ShaderModule.md)

### 10. Graphics pipeline layout

```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
graphicsLayoutDesc.debugName = "Graphics pipeline layout";
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
```

This layout defines a single push constant block: a 16-byte RGBA color available in the fragment stage at offset 0. Unlike the simple triangle example (05), this layout has **no resource bindings** because the triangle is procedurally generated without textures.

**Reference:** [PipelineLayout API](../../api/gpu/PipelineLayout.md)

### 11. Compute pipeline layout

```cpp
auto computeLayout = device->createPipelineLayout({ .debugName = "Compute pipeline layout" });
```

The compute layout is empty -- no push constants or resource bindings. The compute shader in this example is a "noop" that does nothing, included only to demonstrate compute dispatch in an offscreen context.

**Reference:** [PipelineLayout API](../../api/gpu/PipelineLayout.md)

### 12. Graphics pipeline creation

```cpp
auto colorFormat = surface->colorFormat();
auto depthFormat = surface->depthFormat();

gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout = graphicsLayout.get();
pipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat = colorFormat;
pipelineDesc.depthFormat = depthFormat;
pipelineDesc.debugName = "Main graphics pipeline";

auto pipeline = device->createGraphicsPipeline(pipelineDesc);
```

The graphics pipeline description ties together:

| Field | Value | Meaning |
|-------|-------|---------|
| `vertexShader` | `vs` | Vertex shader module |
| `fragmentShader` | `fs` | Fragment shader module |
| `layout` | `graphicsLayout` | Push constants only |
| `topology` | `TriangleList` | Every 3 vertices form a triangle |
| `colorFormat` | Surface color format | Must match the offscreen image format |
| `depthFormat` | Surface depth format | Must match the depth image format |

Note that `vertexBufferDescriptions` is **not** set. This means the pipeline uses procedurally generated vertices from the vertex shader (no vertex buffer input).

**Reference:** [GraphicsPipeline API](../../api/gpu/GraphicsPipeline.md)

### 13. Compute pipeline creation

```cpp
gpu::ComputePipelineDescription computePipelineDesc{};
computePipelineDesc.computeShader = cs.get();
computePipelineDesc.layout = computeLayout.get();
computePipelineDesc.debugName = "Noop compute pipeline";
auto computePipeline = device->createComputePipeline(computePipelineDesc);
```

Simpler than the graphics pipeline -- just a compute shader and a layout. The compute pipeline is used to dispatch the noop shader.

**Reference:** [ComputePipeline API](../../api/gpu/ComputePipeline.md)

### 14. Single render iteration

```cpp
auto& graphicsQueue = device->graphicsQueue();

PushConstants push{};
push.color[0] = 1.0f; push.color[1] = 0.4f; push.color[2] = 0.1f; push.color[3] = 1.0f;
gpu::Color clearColor{ 0.1f, 0.1f, 0.15f, 1.0f };

auto frame = surface->beginFrame();
auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

cmd->begin();
```

This sets up the render iteration:

1. **Push constants** -- an orange-ish color (R=1.0, G=0.4, B=0.1, A=1.0) is pushed to the fragment shader.
2. **Clear color** -- a dark blue-gray (0.1, 0.1, 0.15) is used to clear the background.
3. **Frame acquisition** -- `surface->beginFrame()` returns a single `SurfaceFrame` for offscreen surfaces (unlike windowed surfaces which rotate through 2-3 frames).
4. **Command buffer** -- a command buffer is created from the graphics queue for recording GPU commands.

### 15. Compute dispatch

```cpp
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
cmd->dispatch(1, 1, 1);
cmd->endCompute();
```

This dispatches the noop compute shader with a single work group. The compute shader does nothing (empty `main()`), but this demonstrates how to exercise the compute pipeline path in an offscreen context. In a real application, you might use compute for image processing, physics simulations, or other GPU computations.

**Reference:** [CommandBuffer API -- dispatch()](../../api/gpu/CommandBuffer.md)

### 16. Graphics rendering

```cpp
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearColor(0, clearColor);
cmd->clearDepth(1.0f);
cmd->bindPipeline(pipeline.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
cmd->draw(3);
cmd->endRendering();
```

This renders the triangle:

1. **Transition to ColorAttachment** -- the color image must be in `ColorAttachment` layout for rendering.
2. **Transition to DepthAttachment** -- the depth image must be in `DepthAttachment` layout for depth testing.
3. **Begin rendering** -- starts a dynamic rendering pass using the frame's color and depth images.
4. **Clear color** -- fills the background with the dark blue-gray color.
5. **Clear depth** -- initializes the depth buffer to 1.0 (far plane).
6. **Bind pipeline** -- selects the vertex/fragment shaders and their configuration.
7. **Push constants** -- uploads the orange color to the fragment shader.
8. **Draw 3 vertices** -- issues a non-indexed draw call for 3 vertices. The vertex shader generates vertices procedurally (no vertex buffer needed).
9. **End rendering** -- finalizes the rendering pass.

**Important:** There is no transition to `Present` layout. Offscreen surfaces keep the color image in `ColorAttachment` layout for pixel readback.

**Reference:** [CommandBuffer API -- beginRendering(), draw()](../../api/gpu/CommandBuffer.md)

### 17. Presentation and frame completion

```cpp
surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());
```

For offscreen surfaces, `present()` records the presentation command (which is a no-op for offscreen targets), then the command buffer is ended and submitted to the GPU. `endFrame()` finalizes the frame.

### 18. Wait for GPU completion

```cpp
device->waitIdle();
```

Before reading pixels back to the CPU, we must wait for the GPU to finish all pending operations. `waitIdle()` blocks until the GPU is idle.

### 19. Pixel readback and image saving

```cpp
const uint32_t w = surface->width();
const uint32_t h = surface->height();
std::vector<uint8_t> pixels;
frame->colorImage()->readPixelsRGBA8(pixels, gpu::ImageLayout::ColorAttachment);

auto outPath = std::filesystem::current_path() / "out.jpg";
bg2e::db::saveImage(outPath, pixels.data(), w, h, 4);
std::cout << "Wrote " << outPath << " (" << w << "x" << h << ")\n";
```

This reads the rendered pixels from the GPU:

1. **`readPixelsRGBA8()`** -- reads the color image pixels into a `std::vector<uint8_t>` in RGBA8 format. The image must be in `ColorAttachment` layout (which it is, since we never transitioned to `Present`).
2. **`db::saveImage()`** -- saves the pixel data to a JPEG file. The function takes the path, pixel data, width, height, and bytes per pixel (4 for RGBA).

**Reference:** [Image API -- readPixelsRGBA8()](../../api/gpu/Image.md)

### 20. Cleanup

```cpp
computePipeline->cleanup();
pipeline->cleanup();
computeLayout->cleanup();
graphicsLayout->cleanup();
cs->cleanup();
vs->cleanup();
fs->cleanup();
surface->cleanup();
device->cleanup();
instance->cleanup();
```

Resources are released in **reverse creation order**:

| Step | Action | Reason |
|------|--------|--------|
| 1 | `computePipeline->cleanup()` | Compute pipeline depends on compute layout |
| 2 | `pipeline->cleanup()` | Graphics pipeline depends on graphics layout |
| 3 | `computeLayout->cleanup()` | Compute layout depends on device |
| 4 | `graphicsLayout->cleanup()` | Graphics layout depends on device |
| 5 | `cs->cleanup()` | Compute shader depends on device |
| 6 | `vs->cleanup()` | Vertex shader depends on device |
| 7 | `fs->cleanup()` | Fragment shader depends on device |
| 8 | `surface->cleanup()` | Offscreen surface depends on device |
| 9 | `device->cleanup()` | Logical device depends on instance |
| 10 | `instance->cleanup()` | Instance depends on backend |

**Reference:** [Cleanup order](../../api/gpu/index.md#cleanup-order)

## Shader code explanation

### Vertex shader (`triangle.vert.glsl`)

```glsl
#version 450

layout(location = 0) out vec3 fragColor;

void main()
{
    vec2 positions[3] = vec2[](
        vec2( 0.0, -0.5),
        vec2( 0.5,  0.5),
        vec2(-0.5,  0.5)
    );

    vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

This is a procedural vertex shader that generates a triangle without any vertex buffer input:

- **`gl_VertexIndex`** -- the built-in vertex index (0, 1, 2 for a 3-vertex draw call).
- **Positions** -- three hardcoded positions forming an upward-pointing triangle:
  - Vertex 0: (0.0, -0.5) -- top center
  - Vertex 1: (0.5, 0.5) -- bottom right
  - Vertex 2: (-0.5, 0.5) -- bottom left
- **Colors** -- RGB values for each vertex (red, green, blue) that are interpolated across the triangle.
- **`gl_Position`** -- sets the clip-space position directly (no transformation needed since the geometry is already in normalized device coordinates).
- **`fragColor`** -- passes the vertex color to the fragment shader via rasterization.

The triangle vertices are in the range [-0.5, 0.5], which maps to the visible area of the screen (NDC range [-1, 1]).

### Fragment shader (`triangle.frag.glsl`)

```glsl
#version 450

layout(push_constant) uniform Push {
    vec4 color;
} pc;

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor * pc.color.rgb, pc.color.a);
}
```

Key concepts:

- **Push constants** -- `pc.color` receives the orange color pushed from the CPU each frame.
- **Color modulation** -- the interpolated vertex color (RGB) is multiplied by the push constant color (RGB). This tints the rainbow triangle with the orange color.
- **Alpha** -- the push constant alpha (1.0) is used directly as the output alpha.
- **Input/Output** -- `fragColor` is the interpolated vertex color from the vertex shader; `outColor` is the final framebuffer output.

### Compute shader (`noop.comp.glsl`)

```glsl
#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main()
{
}
```

This is an intentionally empty compute shader. It demonstrates the compute pipeline path without doing any actual work. Key points:

- **Local size** -- `layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;` defines a work group of size 1x1x1. Each invocation handles a single thread.
- **Empty `main()`** -- the shader does nothing. In a real application, you would perform computations here (image processing, physics, etc.).

The compute shader is included to demonstrate that compute and graphics pipelines can coexist in the same offscreen rendering context.

## Key concepts

### Offscreen rendering workflow

The offscreen workflow follows a simplified version of the windowed rendering pipeline:

```
1. Initialize GPU (no SDL)
2. Create offscreen surface with fixed dimensions
3. Create shaders and pipelines
4. Single render iteration (no event loop)
5. Read back pixels
6. Save to disk
7. Cleanup
```

The key difference is that there is no continuous event loop -- you render a single frame and exit. This makes offscreen rendering ideal for:

- **Batch rendering** -- render thousands of frames without a window
- **Server-side rendering** -- generate images on a headless server
- **Testing** -- automated visual regression tests
- **Compute workloads** -- GPU computations without visual output

### Single render iteration

Unlike windowed rendering with a continuous event loop, offscreen rendering performs a single render iteration:

```cpp
auto frame = surface->beginFrame();
auto cmd = graphicsQueue.createCommandBuffer();
cmd->begin();
// ... record commands ...
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());
device->waitIdle();
```

The `device->waitIdle()` call is critical -- it ensures the GPU has finished all operations before reading pixels back to the CPU.

### Pixel readback

Reading pixels from the GPU involves three steps:

1. **Ensure the image is in the correct layout** -- `ColorAttachment` layout for color images.
2. **Call `readPixelsRGBA8()`** -- this copies the pixel data from GPU memory to a CPU-side buffer.
3. **Process the data** -- save to disk, display, or perform further computation.

**Important:** The image must be in `ColorAttachment` layout when reading. If you transitioned to `Present` (as in windowed rendering), the readback would fail or return incorrect data.

### Push constants for per-draw data

Push constants are the fastest way to send small amounts of data from the CPU to shaders. This example uses a 16-byte RGBA color that tints the triangle. Push constants are limited to 128 bytes on most GPUs, so use them for small, frequently updated data.

**Reference:** [CommandBuffer API -- pushConstants()](../../api/gpu/CommandBuffer.md)

### Procedural geometry

The vertex shader generates triangle vertices procedurally using `gl_VertexIndex`. This eliminates the need for vertex buffers and simplifies the rendering pipeline. Use procedural geometry for:

- Simple shapes (triangles, quads, full-screen passes)
- Debug visualization
- GPU-driven rendering where vertices are computed in shaders

### Image layout transitions

Every image operation requires the image to be in a specific layout. This example uses two layouts for the color image:

| Layout | Used by |
|--------|---------|
| `ColorAttachment` | Graphics fragment shader output and pixel readback |

The transition to `ColorAttachment` happens before rendering, and the image stays in that layout for readback. There is no transition to `Present` because we never display the image on screen.

**Reference:** [CommandBuffer API -- transition()](../../api/gpu/CommandBuffer.md)

## Complete source

For reference, here is the full source of `examples/gpu/06_offscreen_triangle/src/main.cpp`:

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <bg2e/db/image.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

struct PushConstants {
    float color[4];
};

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create headless GPU instance (no SDL window)
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create();

    // 4. Create offscreen surface
    const gpu::Size2D size{ 800, 600 };
    std::shared_ptr<gpu::Surface> surface = backend->createOffscreenSurface(instance, size);

    // 5. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 6. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // 7. Create shader modules and pipelines
    auto shaderBasePath = base::PlatformTools::shaderPath();
    std::string targetName = "gpu_offscreen_triangle";

    std::shared_ptr<gpu::ShaderModule> vs;
    std::shared_ptr<gpu::ShaderModule> fs;
    std::shared_ptr<gpu::ShaderModule> cs;

    if (backendType == gpu::BackendType::Vulkan)
    {
        auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
        auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
        auto csPath = (shaderBasePath / targetName / "noop.comp.spv").string();
        vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
        fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
        cs = device->createShaderModule({ csPath, "main", gpu::ShaderStage::Compute,  "Noop compute shader" });
    }
    else
    {
        auto vsPath = (shaderBasePath / targetName / "triangle.vert.metallib").string();
        auto fsPath = (shaderBasePath / targetName / "triangle.frag.metallib").string();
        auto csPath = (shaderBasePath / targetName / "noop.comp.metallib").string();
        vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Triangle vertex shader" });
        fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
        cs = device->createShaderModule({ csPath, "compMain", gpu::ShaderStage::Compute,  "Noop compute shader" });
    }

    // Graphics layout with push constant range for fragment stage
    gpu::PipelineLayoutDescription graphicsLayoutDesc{};
    graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
    graphicsLayoutDesc.debugName = "Graphics pipeline layout";
    auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);

    // Compute layout (empty, no push constants or bindings)
    auto computeLayout = device->createPipelineLayout({ .debugName = "Compute pipeline layout" });

    // Get attachment formats from surface
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout = graphicsLayout.get();
    pipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat = colorFormat;
    pipelineDesc.depthFormat = depthFormat;
    pipelineDesc.debugName = "Main graphics pipeline";

    auto pipeline = device->createGraphicsPipeline(pipelineDesc);

    // Compute pipeline
    gpu::ComputePipelineDescription computePipelineDesc{};
    computePipelineDesc.computeShader = cs.get();
    computePipelineDesc.layout = computeLayout.get();
    computePipelineDesc.debugName = "Noop compute pipeline";
    auto computePipeline = device->createComputePipeline(computePipelineDesc);

    // 8. Single render iteration (no loop — headless offscreen)
    auto& graphicsQueue = device->graphicsQueue();

    PushConstants push{};
    push.color[0] = 1.0f; push.color[1] = 0.4f; push.color[2] = 0.1f; push.color[3] = 1.0f;
    gpu::Color clearColor{ 0.1f, 0.1f, 0.15f, 1.0f };

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    cmd->begin();

    // Compute dispatch (exercises the compute path offscreen)
    cmd->beginCompute();
    cmd->bindPipeline(computePipeline.get());
    cmd->dispatch(1, 1, 1);
    cmd->endCompute();

    // Graphics rendering
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, clearColor);
    cmd->clearDepth(1.0f);
    cmd->bindPipeline(pipeline.get());
    cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
    cmd->draw(3);
    cmd->endRendering();
    // No transition to Present — offscreen surface leaves color in ColorAttachment

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());

    // Wait for GPU to finish — offscreen submit has no fence
    device->waitIdle();

    // 9. Read back pixels and save to disk
    const uint32_t w = surface->width();
    const uint32_t h = surface->height();
    std::vector<uint8_t> pixels;
    frame->colorImage()->readPixelsRGBA8(pixels, gpu::ImageLayout::ColorAttachment);

    auto outPath = std::filesystem::current_path() / "out.jpg";
    bg2e::db::saveImage(outPath, pixels.data(), w, h, 4);
    std::cout << "Wrote " << outPath << " (" << w << "x" << h << ")\n";

    // 10. Cleanup
    computePipeline->cleanup();
    pipeline->cleanup();
    computeLayout->cleanup();
    graphicsLayout->cleanup();
    cs->cleanup();
    vs->cleanup();
    fs->cleanup();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();

    return 0;
}
```

## Building and running

Build the example with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_offscreen_triangle
```

Then run the binary from the `bin/` directory. On macOS you will be prompted to choose a backend. You should see output like:

```
Select backend [1=Metal, 2=Vulkan]: 2
Wrote /path/to/out.jpg (800x600)
```

The program renders a single triangle offscreen and saves the result as `out.jpg` in the current directory. The triangle has RGB vertex colors (red, green, blue) tinted with an orange push constant color, on a dark blue-gray background.

## Next steps

- **[07_uniform_buffers](07_uniform_buffers.md)** -- Add per-frame uniform buffers for dynamic data.
- **[08_render_to_texture](08_render_to_texture.md)** -- Render to an offscreen texture and use it as input.
- **[GPU API Quick Start -- Recipe 6](../../api/gpu/quick_start.md)** -- Offscreen triangle and readback recipe.
- **[Image API](../../api/gpu/Image.md)** -- Full reference for image operations including `readPixelsRGBA8()`.