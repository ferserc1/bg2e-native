# bg2e::gpu Quick Start Guide

A practical, recipe-oriented guide to the `bg2e::gpu` backend abstraction layer.
This document covers every class in the namespace with a brief description and
provides working code examples organized from basic initialization to advanced
rendering techniques.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Class Catalog](#class-catalog)
3. [Recipe 1: Creating a Backend and Instance](#recipe-1-creating-a-backend-and-instance)
4. [Recipe 2: Windowed Device Setup](#recipe-2-windowed-device-setup)
5. [Recipe 3: Offscreen Device Setup](#recipe-3-offscreen-device-setup)
6. [Recipe 4: Clear Loop and Presentation](#recipe-4-clear-loop-and-presentation)
7. [Recipe 5: Simple Triangle with Pipelines and Shaders](#recipe-5-simple-triangle-with-pipelines-and-shaders)
8. [Recipe 6: Offscreen Triangle and Readback](#recipe-6-offscreen-triangle-and-readback)
9. [Recipe 7: Uniform Buffers and Multi-Set Rendering](#recipe-7-uniform-buffers-and-multi-set-rendering)
10. [Recipe 8: Render to Texture](#recipe-8-render-to-texture)
11. [Recipe 9: Cubemap Rendering](#recipe-9-cubemap-rendering)
12. [Cleanup Order](#cleanup-order)

---

## Architecture Overview

`bg2e::gpu` is a low-level, backend-agnostic GPU abstraction layer that targets
multiple graphics APIs (Vulkan and Metal) through a unified interface. It uses
the **Abstract Factory** pattern: application code programs against abstract
interfaces (`bg2e::gpu::*`), and concrete backends (`bg2e::gpu::vk::*` and
`bg2e::gpu::metal::*`) provide the implementation.

The layer is organized into three tiers:

1. **Abstract interfaces** — pure virtual classes that define the contract.
2. **Concrete backends** — Vulkan and Metal implementations that expose
   additional raw-handle accessors when needed.
3. **`Factory`** — entry point that creates the active backend singleton based
   on a `BackendType` enum.

### Creation flow

```
Factory::init(BackendType)
  -> Backend::sharedInstance()            // singleton Instance
  -> Backend::createWindowSurface()       // or createOffscreenSurface()
  -> Backend::createPhysicalDevice()
  -> Backend::createDevice()
       -> device->createShaderModule()
       -> device->createPipelineLayout()
       -> device->createGraphicsPipeline()
       -> device->createComputePipeline()
       -> device->createImage()
       -> device->createSampler()
       -> device->createResourceSet()
       -> device->createBuffer()
       -> device->createCubeMap()
       -> device->immediateSubmit()
```

Command buffers are created from queues:

```
device->graphicsQueue().createCommandBuffer()
```

### Backends

| Backend | `BackendType`         | `WindowType`          | Platform     |
|---------|-----------------------|-----------------------|--------------|
| Vulkan  | `BackendType::Vulkan` | `WindowType::Vulkan`  | All          |
| Metal   | `BackendType::Metal`  | `WindowType::Metal`   | macOS only   |

---

## Class Catalog

Brief descriptions of every class and type in `bg2e::gpu`:

### Core infrastructure

| Class | Header | Description |
|-------|--------|-------------|
| **`Factory`** | `Factory.hpp` | Static entry point. `Factory::init(type)` creates the backend; `Factory::backend()` returns the `Backend*` singleton. |
| **`Backend`** | `Backend.hpp` | Abstract factory for all GPU objects. Creates `Instance`, `PhysicalDevice`, `Device`, `WindowSurface`, `OffscreenSurface`, and `ShaderLib`. |
| **`Instance`** | `Instance.hpp` | Abstract GPU instance. Manages debug mode, application name, and SDL window attachment. Created lazily as a singleton by `Backend`. |
| **`PhysicalDevice`** | `PhysicalDevice.hpp` | Abstract physical GPU device. Enumerates and selects the best GPU via a scoring algorithm. Exposes `PhysicalDeviceProperties` (name, type, memory, ray tracing capabilities). |
| **`Device`** | `Device.hpp` | Abstract logical device. Factory for all GPU resources (`Buffer`, `Image`, `CubeMap`, `ShaderModule`, `PipelineLayout`, `GraphicsPipeline`, `ComputePipeline`, `Sampler`, `ResourceSet`, `RayTracingMesh`, `RayTracingScene`). Provides `graphicsQueue()`, `presentQueue()`, `transferQueue()`, and `immediateSubmit()`. |
| **`Queue`** | `Queue.hpp` | Abstract command queue. Creates `CommandBuffer` objects and submits them for execution. |
| **`CommandBuffer`** | `CommandBuffer.hpp` | Abstract command buffer. Records GPU commands: transitions, rendering passes, compute passes, draws, dispatches, push constants, resource set bindings, and ray tracing acceleration structure builds (`buildRayTracingMesh()`, `buildRayTracingScene()`). |

### Surfaces and presentation

| Class | Header | Description |
|-------|--------|-------------|
| **`Surface`** | `Surface.hpp` | Abstract rendering surface. Base class for windowed and offscreen targets. Manages color/depth images, frame acquisition (`beginFrame()`), presentation (`present()`, `endFrame()`), and resize. |
| **`WindowSurface`** | `WindowSurface.hpp` | Abstract window-backed surface. Inherits `Surface`; `isOffscreen()` returns false. |
| **`OffscreenSurface`** | `OffscreenSurface.hpp` | Offscreen rendering surface. Inherits `Surface`; `isOffscreen()` returns true. Used for headless rendering without an OS window. |
| **`SurfaceFrame`** | `SurfaceFrame.hpp` | Represents a single acquired frame from a surface. Provides access to the current `colorImage()` and `depthImage()`. |

### Resources

| Class | Header | Description |
|-------|--------|-------------|
| **`Image`** | `Image.hpp` | Abstract GPU image. Supports 2D and cubemap types. Methods: `uploadRGBA8()`, `uploadImage()`, `readPixelsRGBA8()`, layout tracking via `currentLayout()`. |
| **`CubeMap`** | `CubeMap.hpp` | Abstract cubemap resource. Wraps an `Image` with 6 faces. Created via `Device::createCubeMap()`. Provides `image()` accessor for the underlying cubemap image. |
| **`Buffer`** | `Buffer.hpp` | Abstract GPU buffer. Two allocation strategies: device-local (vertex/index via staging) and host-visible (uniform/storage via direct CPU write). Methods: `createVertexBuffer()`, `createIndexBuffer()`, `createUniformBuffer()`, `updateUniformBuffer()`, etc. |
| **`Sampler`** | `Sampler.hpp` | Abstract texture sampler. Created via `Device::createSampler()` with a `SamplerDescription` (filter modes, address modes). |
| **`ResourceSet`** | `ResourceSet.hpp` | Abstract descriptor/resource set. Binds images, samplers, buffers, and ray tracing scenes to shader bindings. Methods: `setSampledImage()`, `setSampler()`, `setUniformBuffer()`, `setStorageBuffer()`, `setStorageImage()`, `setSampledCubeMap()`, `setRayTracingScene()`, `update()`. |

### Shaders and pipelines

| Class | Header | Description |
|-------|--------|-------------|
| **`ShaderModule`** | `ShaderModule.hpp` | Abstract compiled shader module. Created via `Device::createShaderModule()` with a `ShaderModuleDescription` (file path, entry point, stage). |
| **`ShaderLib`** | `ShaderLib.hpp` | Convenience loader for shader libraries. Given a base path and backend type, loads `.spv` (Vulkan) or `.metallib` (Metal) files by name. Methods: `vertex()`, `fragment()`, `compute()`. |
| **`PipelineLayout`** | `PipelineLayout.hpp` | Abstract pipeline layout. Defines push constant ranges and resource bindings. Created via `Device::createPipelineLayout()` with a `PipelineLayoutDescription`. |
| **`GraphicsPipeline`** | `GraphicsPipeline.hpp` | Abstract graphics pipeline. Created via `Device::createGraphicsPipeline()` with a `GraphicsPipelineDescription` (shaders, layout, topology, formats, cull mode, vertex buffer descriptions). |
| **`ComputePipeline`** | `ComputePipeline.hpp` | Abstract compute pipeline. Created via `Device::createComputePipeline()` with a `ComputePipelineDescription` (compute shader, layout). |

### Mesh and geometry

| Class | Header | Description |
|-------|--------|-------------|
| **`MeshGeneric<MeshT>`** | `Mesh.hpp` | Template class that wraps a `bg2e::geo` mesh with GPU vertex/index buffers. Methods: `setMeshData()`, `build()`, `draw()`, `drawSubmesh()`, `cleanup()`. Provides a static `vertexBufferDescription()` for pipeline creation and `rayTracingMeshDescription(submeshIndex)` to build a `RayTracingMesh` from the existing buffers. |
| **`MeshP`**, **`MeshPN`**, **`MeshPC`**, **`MeshPU`**, **`MeshPNU`**, **`MeshPNC`**, **`MeshPNUC`**, **`MeshPNUT`**, **`MeshPNUUT`**, **`Mesh`** | `Mesh.hpp` | Type aliases for common vertex layouts (P=position, N=normal, C=color, U=texcoord, T=tangent). `Mesh` is an alias for `MeshPNUUT`. |

### Ray tracing

Acceleration structures and ray queries only — no ray tracing pipeline or shader
binding table. Requires `PhysicalDeviceProperties::rayTracingSupported()`.

| Class | Header | Description |
|-------|--------|-------------|
| **`RayTracingMesh`** | `RayTracingMesh.hpp` | Abstract bottom-level acceleration structure (BLAS) for one submesh, built from existing GPU vertex/index buffers. Created via `Device::createRayTracingMesh()`; built with `CommandBuffer::buildRayTracingMesh()`. |
| **`RayTracingScene`** | `RayTracingScene.hpp` | Abstract top-level acceleration structure (TLAS) holding instances of `RayTracingMesh`. Methods: `clearInstances()`, `addInstance()`, `buildOrUpdate()`. Bound through `ResourceSet::setRayTracingScene()`. |

### Management utilities

| Class | Header | Description |
|-------|--------|-------------|
| **`DeviceResource`** | `DeviceResource.hpp` | Base class for all GPU resources that require explicit cleanup. Provides `isValid()` and `cleanup()` interface. |
| **`CleanupManager`** | `CleanupManager.hpp` | Ordered teardown manager for `DeviceResource` objects. `push()` adds resources; `flush()` cleans them up in reverse order. |
| **`FrameResourceRing<T>`** | `FrameResourceRing.hpp` | Template ring-buffer that creates one `DeviceResource` per swapchain image. `current()` returns the resource for the current frame. Useful for per-frame UBOs and resource sets. |

### Common types (from `Common.hpp`)

| Type | Description |
|------|-------------|
| `BackendType` | Enum: `Vulkan`, `Metal`. |
| `WindowType` | Enum: `Vulkan`, `Metal`. Determines SDL window flags. |
| `PixelFormat` | Enum: color formats (`R8G8B8A8_UNORM`, `B8G8R8A8_UNORM`, `R16G16B16A16_SFLOAT`, etc.) and depth formats (`D32_SFLOAT`, `D24_UNORM_S8_UINT`, etc.). |
| `ImageLayout` | Enum: `Undefined`, `General`, `ColorAttachment`, `DepthAttachment`, `ShaderReadOnly`, `TransferSrc`, `TransferDst`, `Present`. |
| `ShaderStage` | Enum: `Vertex`, `Fragment`, `Compute`. |
| `ResourceType` | Enum: `UniformBuffer`, `StorageBuffer`, `SampledImage`, `StorageImage`, `Sampler`, `AccelerationStructure`. |
| `ShaderBinding` | Struct with `vulkan` and `metal` binding indices. |
| `ResourceBinding` | Struct: set index, `ShaderBinding`, type, stage, count. |
| `PipelineLayoutDescription` | Struct: vector of `PushConstantRange` + vector of `ResourceBinding`. |
| `PushConstantRange` | Struct: offset, size, stage. |
| `SamplerDescription` | Struct: filter modes, address modes, debug name. |
| `ImageDescription` | Struct: size, format, usage, type, mip levels. |
| `ImageUsage` | Bitmask: `ColorAttachment`, `DepthStencil`, `Sampled`, `Storage`, `TransferSrc`, `TransferDst`, `Present`. |
| `BufferUsage` | Bitmask: `Vertex`, `Index`, `Uniform`, `Storage`, `TransferSrc`, `TransferDst`, `AccelerationStructureBuildInput`, `ShaderDeviceAddress`. |
| `RayTracingMeshDescription` | Struct: shared vertex/index buffers, stride, position offset, vertex format, submesh index range. |
| `RayTracingInstance` | Struct: `RayTracingMesh*`, world transform, instance id, mask. |
| `GraphicsPipelineDescription` | Struct: shaders, layout, topology, color/depth formats, cull mode, front face, vertex buffer descriptions. |
| `ComputePipelineDescription` | Struct: compute shader, layout. |
| `VertexAttributeDescription` | Struct: location, binding, semantic, format, offset. |
| `VertexBufferDescription` | Struct: binding, stride, input rate, attributes. |
| `PrimitiveTopology` | Enum: `TriangleList`, `TriangleStrip`, `LineList`, `PointList`. |
| `CullMode` | Enum: `None`, `Front`, `Back`. |
| `FrontFace` | Enum: `Clockwise`, `CounterClockwise`. |
| `CubeMapDescription` | Struct: size, format, usage, mip levels. |
| `CubemapFace` | Enum: `PositiveX`..`NegativeZ` (0-5). |
| `PhysicalDeviceProperties` | Struct: device type, memory, name, vendor, ray tracing capabilities, score. |
| `RayTracingCapabilities` | Struct: availability flags for ray tracing features. |

---

## Recipe 1: Creating a Backend and Instance

The most basic initialization: select a backend and obtain a shared `Instance`.

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>

int main() {
    using namespace bg2e;

    // Select backend (Metal on macOS, Vulkan elsewhere)
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS) {
        backendType = gpu::BackendType::Metal;
    }

    // Initialize the backend and get the singleton instance
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();
    auto* instance = backend->sharedInstance();

    // Instance is ready (no window attached — headless)
    instance->enableDebugMode(true);
    instance->create();

    // Cleanup
    instance->cleanup();
    return 0;
}
```

**Key points:**
- `Factory::init()` must be called before any other GPU call.
- `sharedInstance()` returns a singleton — do not delete it.
- `instance->create()` (no arguments) creates a headless instance.
- `instance->create(window)` attaches to an SDL window for presentation.

---

## Recipe 2: Windowed Device Setup

Create a window, initialize the GPU, select a physical device, and create a
logical device. This is the standard setup for interactive applications.

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>

int main() {
    using namespace bg2e;

    // 1. Select backend
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS) {
        backendType = gpu::BackendType::Metal;
    }

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create SDL window with backend-specific flags
    SDL_Init(SDL_INIT_VIDEO);
    Uint32 windowFlags = (backend->windowType() == gpu::WindowType::Vulkan)
        ? SDL_WINDOW_VULKAN : SDL_WINDOW_METAL;
    SDL_Window* window = SDL_CreateWindow(
        "My App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, windowFlags
    );

    // 4. Create GPU instance attached to the window
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create window surface
    auto surface = backend->createWindowSurface(instance);

    // 6. Select the best physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // Query device properties
    auto props = physicalDevice->properties();
    std::cout << "GPU: " << props->name << std::endl;

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // device is now ready — use it to create resources

    // Cleanup
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

**Key points:**
- Use `backend->windowType()` to select the correct SDL window flags.
- `surface` is returned as a `unique_ptr` but should be stored as `shared_ptr`
  for the frame lifecycle API (`beginFrame()`, `endFrame()`).
- `physicalDevice->choose()` auto-selects the best GPU by score.
- Cleanup must happen in reverse creation order.

---

## Recipe 3: Offscreen Device Setup

For headless rendering (no OS window), skip SDL entirely and use
`OffscreenSurface`.

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>

int main() {
    using namespace bg2e;

    // 1. Select backend
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS) {
        backendType = gpu::BackendType::Metal;
    }

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create headless instance (no SDL window)
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create();

    // 4. Create offscreen surface with fixed dimensions
    auto surface = backend->createOffscreenSurface(instance, gpu::Size2D{800, 600});

    // 5. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 6. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // device is ready for offscreen rendering

    // Cleanup
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    return 0;
}
```

**Key points:**
- `instance->create()` (no arguments) creates a headless instance.
- `createOffscreenSurface()` takes a `Size2D` instead of a window.
- Default offscreen color format is `R8G8B8A8_UNORM` (vs `B8G8R8A8_UNORM` for
  windowed).
- No SDL dependencies required.

---

## Recipe 4: Clear Loop and Presentation

A minimal render loop that clears the screen with an animated color. This
demonstrates the core frame lifecycle.

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <cmath>

int main() {
    using namespace bg2e;

    // Backend + window + instance + surface + device setup (see Recipe 2)
    // ...

    auto& graphicsQueue = device->graphicsQueue();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                surface->resize({
                    static_cast<uint32_t>(event.window.data1),
                    static_cast<uint32_t>(event.window.data2)
                });
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
        gpu::Color clearColor{
            0.5f + 0.5f * std::sin(t),
            0.5f + 0.5f * std::sin(t + 2.0f),
            0.5f + 0.5f * std::sin(t + 4.0f),
            1.0f
        };

        // 1. Acquire next swapchain image
        auto frame = surface->beginFrame();

        // 2. Create and record command buffer
        auto cmd = graphicsQueue.createCommandBuffer("Frame");
        cmd->begin();

        // 3. Transition images to attachment layouts
        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);

        // 4. Begin dynamic rendering, set clear values, end immediately
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, clearColor);
        cmd->clearDepth(1.0f);
        cmd->endRendering();

        // 5. Transition to present layout
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        // 6. Record present operation
        surface->present(cmd.get());

        // 7. End recording and submit
        cmd->end();
        graphicsQueue.submit(cmd.get());

        // 8. Present to screen
        surface->endFrame(frame.get());
    }

    // Cleanup ...
    return 0;
}
```

**Frame lifecycle:**

```
surface->beginFrame()          // acquire swapchain image
  cmd = queue.createCommandBuffer()
  cmd->begin()
    cmd->transition(...)       // layout transitions
    cmd->beginRendering(frame)
    cmd->clearColor(...)
    cmd->clearDepth(...)
    cmd->endRendering()
    cmd->transition(..., Present)
    surface->present(cmd)      // record present
  cmd->end()
  queue.submit(cmd)            // submit to GPU
surface->endFrame(frame)       // present to screen
```

**Key points:**
- `beginRendering()` must be preceded by `clearColor()` / `clearDepth()` calls.
- `transition()` must be called before using an image in a new layout.
- The final transition before presentation must be to `ImageLayout::Present`.
- Handle `SDL_WINDOWEVENT_RESIZED` to call `surface->resize()`.

---

## Recipe 5: Simple Triangle with Pipelines and Shaders

This example renders a textured pentagon with a compute-generated gradient
background. It demonstrates shaders, pipeline layouts, resource sets, push
constants, textures, samplers, meshes, and compute pipelines.

### Shader code

**Vertex shader** (`triangle.vert.glsl`):

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

**Fragment shader** (`triangle.frag.glsl`):

```glsl
#version 450

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler   uSampler;

layout(push_constant) uniform Push {
    vec4 color;
} pc;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(sampler2D(uTex, uSampler), fragUV);
    outColor = vec4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}
```

**Compute shader** (`gradient.comp.glsl`):

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform image2D outImage;

void main() {
    ivec2 size = imageSize(outImage);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= size.x || pos.y >= size.y) return;
    vec2 uv = vec2(float(pos.x) / float(size.x), float(pos.y) / float(size.y));
    imageStore(outImage, pos, vec4(uv.x, uv.y, 0.5, 1.0));
}
```

### C++ setup

```cpp
struct PushConstants {
    float color[4];
};

// --- Shader modules ---
auto shaderBasePath = base::PlatformTools::shaderPath();

// Vulkan uses .spv files, Metal uses .metallib
std::shared_ptr<gpu::ShaderModule> vs, fs, cs;
if (backendType == gpu::BackendType::Vulkan) {
    vs = device->createShaderModule({
        (shaderBasePath / "target" / "triangle.vert.spv").string(),
        "main", gpu::ShaderStage::Vertex, "VS"
    });
    fs = device->createShaderModule({
        (shaderBasePath / "target" / "triangle.frag.spv").string(),
        "main", gpu::ShaderStage::Fragment, "FS"
    });
    cs = device->createShaderModule({
        (shaderBasePath / "target" / "gradient.comp.spv").string(),
        "main", gpu::ShaderStage::Compute, "CS"
    });
} else {
    // Metal: different paths and entry points
    vs = device->createShaderModule({
        (shaderBasePath / "target" / "triangle.vert.metallib").string(),
        "vertMain", gpu::ShaderStage::Vertex, "VS"
    });
    // ... similar for fs, cs
}

// --- Graphics pipeline layout ---
// Push constant (fragment) + SampledImage + Sampler
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.pushConstants.push_back(
    { 0, sizeof(PushConstants), gpu::ShaderStage::Fragment }
);
graphicsLayoutDesc.resourceBindings.push_back(
    { 0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::SampledImage,
      gpu::ShaderStage::Fragment, 1 }
);
graphicsLayoutDesc.resourceBindings.push_back(
    { 0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::Sampler,
      gpu::ShaderStage::Fragment, 1 }
);
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);

// --- Compute pipeline layout ---
gpu::PipelineLayoutDescription computeLayoutDesc{};
computeLayoutDesc.resourceBindings.push_back(
    { 0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage,
      gpu::ShaderStage::Compute, 1 }
);
auto computeLayout = device->createPipelineLayout(computeLayoutDesc);

// --- Texture + sampler ---
auto texture = device->createImage({
    .size = {2, 2},
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst
});
texture->uploadRGBA8(texels.data(), {2, 2});
device->immediateSubmit([texture](gpu::CommandBuffer* cmd) {
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});

auto sampler = device->createSampler({});

// --- Resource set (texture + sampler) ---
auto textureSet = device->createResourceSet(graphicsLayout.get(), 0);
textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
textureSet->setSampler({.vulkan = 1, .metal = 1}, sampler.get());
textureSet->update();

// --- Compute resource set (storage image) ---
// One per swapchain image, updated each frame
auto imageCount = surface->imageCount();
std::vector<std::shared_ptr<gpu::ResourceSet>> computeSets;
for (uint32_t i = 0; i < imageCount; ++i) {
    auto set = device->createResourceSet(computeLayout.get(), 0);
    // Will be updated per-frame with the current color image
    computeSets.push_back(set);
}

// --- Mesh ---
bg2e::geo::MeshPU meshData;
meshData.vertices = { /* position + texcoord vertices */ };
meshData.indices  = { /* triangle indices */ };
meshData.submeshes = { {0, indexCount} };

gpu::MeshPU mesh;
mesh.setMeshData(meshData);
mesh.build(device.get());

// --- Graphics pipeline ---
gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat    = surface->colorFormat();
pipelineDesc.depthFormat    = surface->depthFormat();
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);

// --- Compute pipeline ---
gpu::ComputePipelineDescription computePipelineDesc{};
computePipelineDesc.computeShader = cs.get();
computePipelineDesc.layout        = computeLayout.get();
auto computePipeline = device->createComputePipeline(computePipelineDesc);
```

### Render loop

```cpp
auto frame = surface->beginFrame();
auto cmd   = graphicsQueue.createCommandBuffer("Frame");
cmd->begin();

// Compute pass: write gradient to color image
cmd->transition(frame->colorImage(), gpu::ImageLayout::General);
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
auto* rs = computeSets[ringIndex].get();
rs->setStorageImage({.vulkan = 0, .metal = 0}, frame->colorImage());
rs->update();
cmd->bindResourceSet(computePipeline.get(), 0, rs);
uint32_t gx = (frame->colorImage()->width()  + 15) / 16;
uint32_t gy = (frame->colorImage()->height() + 15) / 16;
cmd->dispatch(gx, gy, 1);
cmd->endCompute();

// Graphics pass
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearDepth(1.0f);
cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, textureSet.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
mesh.draw(cmd.get());
cmd->endRendering();

cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);
surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());
```

**Key points:**
- Shader binding indices differ between Vulkan and Metal — always specify both
  via `ShaderBinding{.vulkan = N, .metal = M}`.
- `immediateSubmit()` runs a lambda synchronously for one-shot operations like
  texture uploads.
- `mesh.draw(cmd)` internally calls `bindVertexBuffer`, `bindIndexBuffer`, and
  `drawIndexed` per submesh.
- Compute shaders use `image2D` (storage images), not textures.

---

## Recipe 6: Offscreen Triangle and Readback

Render a triangle offscreen (no window) and read back the pixels to save an
image. Demonstrates headless rendering and `readPixelsRGBA8()`.

```cpp
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <bg2e/db/image.hpp>

// Setup: offscreen device + surface (see Recipe 3)
// ...

// Create pipelines (same as Recipe 5, but no compute background)
auto frame = surface->beginFrame();
auto cmd   = graphicsQueue.createCommandBuffer("Frame");
cmd->begin();

// Optional: compute dispatch offscreen
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
cmd->dispatch(1, 1, 1);
cmd->endCompute();

// Graphics rendering
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearColor(0, gpu::Color{0.1f, 0.1f, 0.15f, 1.0f});
cmd->clearDepth(1.0f);
cmd->bindPipeline(pipeline.get());
cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
cmd->draw(3);  // procedural triangle — no vertex buffer needed
cmd->endRendering();
// No transition to Present — offscreen surface keeps color in ColorAttachment

surface->present(cmd.get());
cmd->end();
graphicsQueue.submit(cmd.get());
surface->endFrame(frame.get());

// Wait for GPU to finish
device->waitIdle();

// Read back pixels
std::vector<uint8_t> pixels;
frame->colorImage()->readPixelsRGBA8(pixels, gpu::ImageLayout::ColorAttachment);

// Save to disk
bg2e::db::saveImage("output.jpg", pixels.data(),
    surface->width(), surface->height(), 4);
```

**Key points:**
- `cmd->draw(3)` issues a non-indexed draw call — the vertex shader generates
  vertices procedurally.
- `readPixelsRGBA8()` reads from the specified layout — make sure the image is
  in `ColorAttachment` layout when reading.
- Call `device->waitIdle()` before reading pixels to ensure the GPU has finished.
- Offscreen surfaces do not need a transition to `Present`.

---

## Recipe 7: Uniform Buffers and Multi-Set Rendering

Render a rotating textured cube with per-frame uniform buffers. Demonstrates
`Buffer` (UBO), `FrameResourceRing`, multi-descriptor-set layouts, and
`CleanupManager`.

### UBO structures

```cpp
struct CameraUBO {
    glm::mat4 projectionView;
};

struct ModelUBO {
    glm::mat4 model;
};
```

### Pipeline layout with 3 descriptor sets

```cpp
gpu::PipelineLayoutDescription desc{};
// set 0: camera UBO (vertex)
desc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 1: model UBO (vertex)
desc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
// set 2: texture + sampler (fragment)
desc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1
});
desc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 1},
    gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment, 1
});
auto layout = device->createPipelineLayout(desc);
```

### Persistent camera UBO (set 0)

```cpp
CameraUBO cameraData{};
cameraData.projectionView = projection * view;

auto cameraUbo = device->createBuffer("Camera UBO");
cameraUbo->createUniformBuffer(cameraData);

auto cameraSet = device->createResourceSet(layout.get(), 0);
cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
cameraSet->update();
```

### Per-frame model UBO ring (set 1)

```cpp
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buf = device->createBuffer("Model UBO " + std::to_string(i));
    buf->createUniformBuffer(ModelUBO{});
    return buf;
});

gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(layout.get(), 1);
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
    set->update();
    return set;
});
```

### Render loop

```cpp
const float t = /* time */;
ModelUBO modelData{};
modelData.model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.4f, 1.0f, 0.2f));

auto* modelUbo = modelUboRing.current();
modelUbo->updateUniformBuffer(modelData);  // host-visible — direct memcpy

auto frame = surface->beginFrame();
auto cmd   = graphicsQueue.createCommandBuffer("Frame");
cmd->begin();

cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
cmd->clearDepth(1.0f);

cmd->bindPipeline(pipeline.get());
cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
cmd->bindResourceSet(pipeline.get(), 1, modelSetRing.current());
cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
cube.draw(cmd.get());

cmd->endRendering();
// ... present and submit
```

### CleanupManager

```cpp
gpu::CleanupManager cleanup;
cleanup.push(vs);
cleanup.push(fs);
cleanup.push(graphicsLayout);
cleanup.push(pipeline);
cleanup.push(texture);
cleanup.push(sampler);
cleanup.push(textureSet);
cleanup.push(cameraUbo);
cleanup.push(cameraSet);

// At shutdown:
device->waitIdle();
modelSetRing.cleanup();
modelUboRing.cleanup();
cube.cleanup();
cleanup.flush();  // cleans up in reverse push order
```

**Key points:**
- `createUniformBuffer()` allocates host-visible memory — `updateUniformBuffer()`
  does a direct `memcpy`, no staging buffer.
- `FrameResourceRing` creates one resource per swapchain image. Use `current()`
  to get the resource for the current frame.
- `cleanup.push()` takes `shared_ptr<DeviceResource>`. `flush()` cleans up in
  reverse order.
- Metal buffer indices for UBOs must be >= 2 in vertex stage and >= 1 in
  fragment stage (buffer(0) and buffer(1) are reserved).

---

## Recipe 8: Render to Texture

Render a cube into an offscreen color image, run a compute post-processing
pass, then copy the result to the surface for presentation. Demonstrates
rendering to regular `Image` objects, compute read/write, and `copyImage()`.

### Offscreen images

```cpp
// Color attachment + sampled + storage (for compute write)
auto offscreenColor = device->createImage({
    .size = surfaceSize,
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::ColorAttachment
           | gpu::ImageUsage::Sampled
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::Storage
});

// Depth attachment
auto offscreenDepth = device->createImage({
    .size = surfaceSize,
    .format = gpu::PixelFormat::D32_SFLOAT,
    .usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled
});

// Compute output
auto computeOutput = device->createImage({
    .size = surfaceSize,
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::Storage
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::TransferDst
});
```

### Compute resource set (input + output storage images)

```cpp
auto computeSet = device->createResourceSet(computeLayout.get(), 0);
computeSet->setStorageImage({.vulkan = 0, .metal = 0}, offscreenColor.get());
computeSet->setStorageImage({.vulkan = 1, .metal = 1}, computeOutput.get());
computeSet->update();
```

### Render loop with 3 passes

```cpp
cmd->begin();

// Pass 1: Render cube into offscreen color/depth
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

// Pass 2: Compute edge detection
cmd->transition(offscreenColor.get(), gpu::ImageLayout::General);
cmd->transition(computeOutput.get(), gpu::ImageLayout::General);
cmd->beginCompute();
cmd->bindPipeline(computePipeline.get());
cmd->bindResourceSet(computePipeline.get(), 0, computeSet.get());
cmd->dispatch((w + 15) / 16, (h + 15) / 16, 1);
cmd->endCompute();

// Pass 3: Copy result to surface
cmd->transition(computeOutput.get(), gpu::ImageLayout::TransferSrc);
cmd->transition(frame->colorImage(), gpu::ImageLayout::TransferDst);
cmd->copyImage(computeOutput.get(), frame->colorImage());
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

cmd->end();
```

### Compute shader (Sobel edge detection)

```glsl
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba8) uniform readonly image2D inputImage;
layout(set = 0, binding = 1, rgba8) uniform image2D outputImage;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(inputImage);
    if (pos.x >= size.x || pos.y >= size.y) return;

    float tl = luma(imageLoad(inputImage, pos + ivec2(-1, -1)).rgb);
    float t  = luma(imageLoad(inputImage, pos + ivec2( 0, -1)).rgb);
    float tr = luma(imageLoad(inputImage, pos + ivec2( 1, -1)).rgb);
    float l  = luma(imageLoad(inputImage, pos + ivec2(-1,  0)).rgb);
    float r  = luma(imageLoad(inputImage, pos + ivec2( 1,  0)).rgb);
    float bl = luma(imageLoad(inputImage, pos + ivec2(-1,  1)).rgb);
    float b  = luma(imageLoad(inputImage, pos + ivec2( 0,  1)).rgb);
    float br = luma(imageLoad(inputImage, pos + ivec2( 1,  1)).rgb);

    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy = -tl - 2.0*t - tr + bl + 2.0*b + br;
    float edge = clamp(sqrt(gx*gx + gy*gy), 0.0, 1.0);

    imageStore(outputImage, pos, vec4(vec3(edge), 1.0));
}
```

**Key points:**
- `beginRendering()` accepts two `Image*` arguments for offscreen rendering
  (color + depth), instead of a `SurfaceFrame*`.
- Compute shaders use `image2D` for storage images and `imageLoad()`/`imageStore()`.
- `copyImage()` copies between `gpu::Image` objects — both must have
  `TransferSrc` and `TransferDst` usage flags.
- Offscreen images must be recreated on window resize.

---

## Recipe 9: Cubemap Rendering

Render an equirectangular HDR texture into a cubemap, then use it for mirror
reflection on a cube. Demonstrates `CubeMap`, `beginRendering(CubeMap*, face)`,
and cubemap sampling in shaders.

### Cubemap creation

```cpp
auto cubeMap = device->createCubeMap({
    .size = 512,
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled
           | gpu::ImageUsage::ColorAttachment
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::TransferDst,
    .mipLevels = 1
});
```

### Render into cubemap faces

```cpp
struct CubemapFaceCamera {
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;
};

std::array<CubemapFaceCamera, 6> faceCameras = {{
    { glm::vec3(0), glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0) }, // +X
    { glm::vec3(0), glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0) }, // -X
    { glm::vec3(0), glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1) }, // +Y
    { glm::vec3(0), glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1) }, // -Y
    { glm::vec3(0), glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0) }, // +Z
    { glm::vec3(0), glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0) }, // -Z
}};

for (uint32_t face = 0; face < 6; ++face) {
    auto& fc = faceCameras[face];
    glm::mat4 faceView = glm::lookAt(fc.eye, fc.center, fc.up);

    CubemapCameraPushConstants cam{};
    cam.projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    cam.view = faceView;

    cmd->transition(cubeMap->image(), gpu::ImageLayout::ColorAttachment);
    cmd->beginRendering(cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
    cmd->clearColor(0, gpu::Color(0, 0, 0, 1));

    cmd->bindPipeline(cubemapRendererPipeline.get());
    cmd->bindResourceSet(cubemapRendererPipeline.get(), 0, modelSet);
    cmd->bindResourceSet(cubemapRendererPipeline.get(), 1, equirectTextureSet.get());
    cmd->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(cam), &cam);
    sphere.draw(cmd.get());

    cmd->endRendering();
}

// Transition cubemap to shader-readable
cmd->transition(cubeMap->image(), gpu::ImageLayout::ShaderReadOnly);
```

### Bind cubemap as a sampled texture

```cpp
auto cubemapSet = device->createResourceSet(reflectionLayout.get(), 2);
cubemapSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, cubeMap.get());
cubemapSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
cubemapSet->update();
```

### Fragment shader with cubemap sampling

```glsl
#version 450

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler      uSampler;

layout(push_constant) uniform PushConstants {
    vec3 cameraPos;
} push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 I = normalize(fragWorldPos - push.cameraPos);
    vec3 R = reflect(I, N);
    outColor = texture(samplerCube(uCubeMap, uSampler), R);
}
```

**Key points:**
- `beginRendering(cubeMap.get(), face, mipLevel)` renders into a specific
  cubemap face. Use `gpu::CubemapFace` enum values (0-5).
- `CubeMap::image()` returns the underlying cubemap `Image*` for transitions.
- Use `setSampledCubeMap()` (not `setSampledImage()`) to bind a cubemap to a
  `textureCube` sampler in the shader.
- The cubemap pipeline must use `R16G16B16A16_SFLOAT` as its color format to
  match the cubemap's pixel format.
- Set `cullMode = CullMode::None` for the cubemap renderer since you're
  rendering from inside the sphere.

---

## Cleanup Order

Resources must be released in reverse creation order to avoid dangling
references:

```
1.  device->waitIdle()
2.  Mesh buffers (mesh.cleanup())
3.  FrameResourceRing cleanup (modelSetRing, modelUboRing, etc.)
4.  CleanupManager::flush() — DeviceResource objects in reverse push order
5.  surface->cleanup()
6.  device->cleanup()
7.  instance->cleanup()
8.  SDL_DestroyWindow() / SDL_Quit() (if applicable)
```

**Key points:**
- Always call `device->waitIdle()` before cleanup.
- `CleanupManager::flush()` handles ordered teardown for resources pushed to it.
- `FrameResourceRing::cleanup()` must be called before `CleanupManager::flush()`
  if the ring resources depend on other managed resources.
- `Mesh` objects are not `DeviceResource` subclasses — call `mesh.cleanup()`
  manually.
