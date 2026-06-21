# Tutorial 07: Uniform Buffers

This tutorial walks through the `07_uniform_buffers` example: rendering a textured rotating cube using multiple descriptor sets and per-frame uniform buffers. This is the foundational example for passing dynamic data from the CPU to shaders through the GPU abstraction layer.

**Source:** `examples/gpu/07_uniform_buffers/src/main.cpp`

## What you will learn

- How to define and use Uniform Buffer Objects (UBOs) for per-frame data
- How to set up a pipeline layout with multiple descriptor sets (set 0, 1, 2)
- How to use `FrameResourceRing` to manage per-frame resources without synchronization issues
- How to create persistent resource sets (camera UBO, texture + sampler) and per-frame resource sets (model UBO)
- How to create and upload a procedural texture with a sampler
- How to build a mesh from procedural geometry and draw it with indexed rendering
- How to update uniform buffers every frame without re-creating descriptor sets
- How `CleanupManager` simplifies ordered teardown of mixed resource types

## Prerequisites

- Completed [05_simple_triangle](05_simple_triangle.md) -- you should understand pipeline creation and basic rendering
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding uniform buffers

A Uniform Buffer Object (UBO) is a GPU memory region that shaders can read from. Unlike push constants (limited to 128 bytes), UBOs can hold larger data structures and are the standard way to pass transformation matrices, camera parameters, and material properties to shaders.

The key challenge with UBOs is **data race prevention**: if the CPU writes to a buffer while the GPU is still reading it, corruption occurs. The `FrameResourceRing` solves this by maintaining one buffer per in-flight frame, so the CPU always writes to a buffer the GPU is done reading.

| Data type | Recommended transport | Size limit |
|-----------|----------------------|------------|
| Small constants (color, scale) | Push constants | 128 bytes |
| Transformation matrices | Uniform buffer (UBO) | 16 KB - 64 KB |
| Large data arrays | Storage buffer (SSBO) | Device-dependent |

**Reference:** [Buffer API -- uniform buffers](../../api/gpu/Buffer.md), [FrameResourceRing API](../../api/gpu/FrameResourceRing.md)

## Step-by-step code explanation

### 1. Headers and UBO structs

```cpp
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>
#include <array>
#include <iostream>
#include <memory>

struct CameraUBO {
    glm::mat4 projectionView;
};

struct ModelUBO {
    glm::mat4 model;
};
```

| Header | Purpose |
|--------|---------|
| `<bg2e.hpp>` | Full engine include |
| `<bg2e/gpu/all.hpp>` | GPU abstraction layer (Factory, Backend, Device, Buffer, etc.) |
| `<bg2e/app/SDLUtils.hpp>` | SDL initialization helpers (`app::initSdlVideoDriver()`) |
| `<bg2e/math/base.hpp>` | GLM math utilities |

The two structs define the data layout that shaders will read:

- **`CameraUBO`** -- a combined projection-view matrix (64 bytes). This is the same data that would normally be split into separate projection and view matrices, but packing them into one `mat4` reduces descriptor set overhead.
- **`ModelUBO`** -- a model transformation matrix (64 bytes). Updated every frame to rotate the cube.

**Important:** UBO struct layouts must match the corresponding GLSL uniform block exactly. Both structs contain a single `glm::mat4` (16 floats = 64 bytes), which maps directly to `mat4` in GLSL.

### 2. Backend selection and SDL initialization

```cpp
auto backendType = gpu::BackendType::Vulkan;
if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
{
    std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
    int choice = 0;
    std::cin >> choice;
    backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
}

gpu::Factory::init(backendType);
auto* backend = gpu::Factory::backend();
```

Same as previous examples: select the backend (Vulkan on Linux/Windows, choice on macOS), initialize the factory, and retrieve the backend singleton.

### 3. Window creation with backend-specific flags

```cpp
app::initSdlVideoDriver();
SDL_Init(SDL_INIT_VIDEO);

Uint32 windowFlags = 0;
switch (backend->windowType())
{
    case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
    case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
}

SDL_Window* window = SDL_CreateWindow(
    "GPU Uniform Buffers Example",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    800, 600,
    windowFlags | SDL_WINDOW_RESIZABLE
);
```

The window is created with `SDL_WINDOW_RESIZABLE` so the example can demonstrate surface resize handling. The `backend->windowType()` method returns the correct window flag for the selected backend.

### 4. Instance, surface, physical device, and logical device

```cpp
auto* instance = backend->sharedInstance();
instance->enableDebugMode(true);
instance->create(window);

std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

auto physicalDevice = backend->createPhysicalDevice();
physicalDevice->choose(*instance, *surface);

auto device = backend->createDevice();
device->create(instance, physicalDevice.get(), surface.get());
```

This is the standard device initialization sequence. The key difference from offscreen examples is that `instance->create(window)` receives an SDL window, and `createWindowSurface` creates a swapchain-backed surface.

### 5. CleanupManager for ordered teardown

```cpp
gpu::CleanupManager cleanup(surface.get());
```

`CleanupManager` tracks all `DeviceResource` objects and cleans them in reverse insertion order when `flush()` is called. This eliminates the need to manually track cleanup order for dozens of resources. Resources are pushed with `cleanup.push(resource)` and released in bulk at shutdown.

**Reference:** [CleanupManager API](../../api/gpu/CleanupManager.md)

### 6. Shader creation via ShaderLib

```cpp
auto shaderBasePath = base::PlatformTools::shaderPath();

auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_uniform_buffers");
auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());
cleanup.push(vs);
cleanup.push(fs);
```

`ShaderLib` simplifies shader loading by locating the correct binary for the active backend:

- On **Vulkan**, it loads `cube.vert.spv` and `cube.frag.spv` from the shader output directory.
- On **Metal**, it loads `cube.vert.metallib` and `cube.frag.metallib`.

The `"cube"` argument is the base name of the shader pair. `ShaderLib` handles the platform-specific file extension and entry point naming automatically.

Both shader modules are pushed to the `CleanupManager` for teardown.

### 7. Pipeline layout with three descriptor sets

```cpp
gpu::PipelineLayoutDescription graphicsLayoutDesc{};
graphicsLayoutDesc.resourceBindings.push_back({
    0,
    {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Vertex,
    1
});
graphicsLayoutDesc.resourceBindings.push_back({
    1,
    {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Vertex,
    1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2,
    {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage,
    gpu::ShaderStage::Fragment,
    1
});
graphicsLayoutDesc.resourceBindings.push_back({
    2,
    {.vulkan = 1, .metal = 1},
    gpu::ResourceType::Sampler,
    gpu::ShaderStage::Fragment,
    1
});
graphicsLayoutDesc.debugName = "Cube pipeline layout";
auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
cleanup.push(graphicsLayout);
```

This is the most important part of the example. The layout declares **three descriptor sets** with four resource bindings:

| Set | Binding | Resource type | Stage | Vulkan binding | Metal index | Purpose |
|-----|---------|---------------|-------|----------------|-------------|---------|
| 0 | 0 | UniformBuffer | Vertex | 0 | buffer(2) | Camera UBO |
| 1 | 0 | UniformBuffer | Vertex | 0 | buffer(3) | Model UBO |
| 2 | 0 | SampledImage | Fragment | 0 | texture(0) | Texture |
| 2 | 1 | Sampler | Fragment | 1 | sampler(0) | Texture sampler |

**Why three sets?** Descriptor sets group resources by update frequency:

- **Set 0 (Camera)** -- updated once at startup, bound every frame. Never changes.
- **Set 1 (Model)** -- updated every frame with a new transformation matrix. Changes per draw call.
- **Set 2 (Material)** -- texture + sampler, updated once at startup. Never changes.

This grouping allows the GPU to efficiently cache descriptor set bindings. When only the model matrix changes, the camera and material sets remain bound.

**Metal binding note:** The `metal` indices for UBOs must start at 2 for vertex stage (index 0 = vertex buffer, index 1 = push constants) and at 1 for fragment stage (index 0 = push constants). This matches the reservation rules in the Metal backend.

**Reference:** [PipelineLayout API](../../api/gpu/PipelineLayout.md)

### 8. Procedural texture and sampler

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
device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
{
    cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
});
cleanup.push(texture);

auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
cleanup.push(sampler);
```

A 2x2 texture with four colored pixels (red, green, blue, yellow) is created procedurally:

1. **`createImage`** -- allocates a 2x2 RGBA8 image with `Sampled` (for shader read) and `TransferDst` (for upload) usage flags.
2. **`uploadRGBA8`** -- copies the texel data into the image via an internal staging buffer.
3. **`immediateSubmit`** -- transitions the image layout to `ShaderReadOnly`. This is required before the image can be sampled in a fragment shader.
4. **`createSampler`** -- creates a default linear (bilinear) sampler for texture filtering.

The texture will appear as four large colored blocks on the cube faces, making the rotation easy to see.

### 9. Camera UBO -- persistent resource set (set 0)

```cpp
const float aspect = 800.0f / 600.0f;
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
```

The camera UBO is created once and never updated:

1. **Projection matrix** -- 60-degree vertical FOV, 800:600 aspect ratio, near plane 0.1, far plane 100.
2. **View matrix** -- camera at (0, 0, 3) looking at the origin with Y-up.
3. **Combined** -- `projection * view` is computed on the CPU and uploaded to the UBO. The vertex shader multiplies this by the model matrix directly.
4. **Resource set** -- set 0 is created once, the buffer is assigned, and `update()` flushes the descriptor to the backend.

Since this data never changes, no `FrameResourceRing` is needed.

**Reference:** [ResourceSet API](../../api/gpu/ResourceSet.md)

### 10. Model UBO -- per-frame FrameResourceRing (set 1)

```cpp
gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
modelUboRing.create(surface.get(), [&](uint32_t i)
{
    auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
modelSetRing.create(surface.get(), [&](uint32_t i)
{
    auto set = device->createResourceSet(graphicsLayout.get(), 1,
        "Model resource set ring[" + std::to_string(i) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
    set->update();
    return set;
});
```

This is the core pattern for per-frame data. Two `FrameResourceRing` instances are created:

**`modelUboRing`** -- one `Buffer` per in-flight frame (typically 2 for window surfaces):
- Each buffer is a host-visible uniform buffer created with `createUniformBuffer`.
- `surface->inFlightFrames()` determines the ring size (2 for windowed, 1 for offscreen).
- `modelUboRing.current()` returns the buffer for the current frame.

**`modelSetRing`** -- one `ResourceSet` per in-flight frame:
- Each set binds the corresponding buffer from `modelUboRing` via `sharedAt(i)`.
- The `shared_ptr` ensures the buffer stays alive as long as the set references it.
- Once `update()` is called, the set is **never re-updated** -- only the buffer contents change.

**Why two rings?** The model UBO changes every frame, so it needs one buffer per in-flight frame to avoid writing to a buffer the GPU is still reading. The resource set that references it must also be duplicated because each set points to a different buffer.

**Reference:** [FrameResourceRing API](../../api/gpu/FrameResourceRing.md)

### 11. Cube mesh from procedural geometry

```cpp
std::unique_ptr<bg2e::geo::MeshPU> cubeData(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));

gpu::MeshPU cube;
cube.setMeshData(*cubeData);
cube.build(device.get());
```

`createCubePU` generates a unit cube with position and texture coordinate attributes (the `PU` suffix means Position + UV). The mesh data is copied into a `gpu::MeshPU` object and built on the device, which uploads vertex data to GPU-local memory.

`MeshPU` provides `vertexBufferDescription()` which is used in the pipeline to declare the vertex input layout.

### 12. Graphics pipeline with depth testing

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
pipelineDesc.debugName      = "Cube graphics pipeline";
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);
cleanup.push(pipeline);
```

The pipeline description ties together:

| Field | Value | Meaning |
|-------|-------|---------|
| `vertexShader` | `vs` | Vertex shader module |
| `fragmentShader` | `fs` | Fragment shader module |
| `layout` | `graphicsLayout` | Three descriptor sets, no push constants |
| `topology` | `TriangleList` | Every 3 vertices form a triangle |
| `colorFormat` | Surface color format | Must match the swapchain image format |
| `depthFormat` | Surface depth format | Enables depth testing |
| `vertexBufferDescriptions` | `MeshPU::vertexBufferDescription()` | Position + UV vertex layout |

Setting `depthFormat` enables depth testing. Without it, faces could render in the wrong order. The depth format must match the surface's depth attachment format.

**Reference:** [GraphicsPipeline API](../../api/gpu/GraphicsPipeline.md)

### 13. Render loop with UBO updates

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
            device->waitIdle();
            surface->resize({
                static_cast<uint32_t>(event.window.data1),
                static_cast<uint32_t>(event.window.data2)
            });
        }
    }

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

    ModelUBO modelData{};
    modelData.model = glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    auto* modelUbo = modelUboRing.current();
    modelUbo->updateUniformBuffer(modelData);
    auto* modelSet = modelSetRing.current();

    cmd->begin();

    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
    cmd->clearDepth(1.0f);

    cmd->bindPipeline(pipeline.get());
    cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
    cmd->bindResourceSet(pipeline.get(), 1, modelSet);
    cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
    cube.draw(cmd.get());

    cmd->endRendering();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());
}
```

Each frame follows this sequence:

1. **Event handling** -- process SDL events including window close and resize. Resize requires `device->waitIdle()` before resizing the surface.
2. **Compute time** -- `SDL_GetTicks64()` provides milliseconds since startup, converted to seconds for smooth rotation.
3. **Update model UBO** -- compute the rotation matrix and write it to the current frame's buffer with `updateUniformBuffer`. This is a direct CPU write to host-visible memory (no staging, no GPU sync).
4. **Begin frame** -- `surface->beginFrame()` acquires the next swapchain image and returns a `SurfaceFrame`.
5. **Record commands** -- transition attachments, begin rendering, clear buffers, bind pipeline and all three resource sets, draw the cube.
6. **Bind resource sets** -- set 0 (camera, persistent), set 1 (model, per-frame), set 2 (texture, persistent).
7. **Present** -- transition to present layout and submit.

**Key insight:** `modelSet` is a raw pointer (not `shared_ptr`) returned by `modelSetRing.current()`. The ring owns the `shared_ptr` and guarantees lifetime through the frame. This avoids unnecessary reference counting overhead in the hot path.

### 14. Cleanup

```cpp
device->waitIdle();

modelSetRing.cleanup();
modelUboRing.cleanup();
cube.cleanup();
cleanup.flush();

surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

Resources are cleaned in this order:

| Step | Action | Reason |
|------|--------|--------|
| 1 | `device->waitIdle()` | Ensure GPU is idle before destroying resources |
| 2 | `modelSetRing.cleanup()` | Per-frame resource sets cleaned first (they reference per-frame buffers) |
| 3 | `modelUboRing.cleanup()` | Per-frame buffers cleaned after their sets |
| 4 | `cube.cleanup()` | Mesh vertex/index buffers released |
| 5 | `cleanup.flush()` | All pushed resources cleaned in reverse insertion order |
| 6 | `surface->cleanup()` | Swapchain released |
| 7 | `device->cleanup()` | Logical device released |
| 8 | `instance->cleanup()` | Instance released |

The `FrameResourceRing` instances are cleaned explicitly before `CleanupManager::flush()` because they hold resources that reference each other. The ring's `cleanup()` method calls `cleanup()` on all slots in reverse order.

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

Key concepts:

- **Vertex inputs** -- `inPosition` (location 0, vec3) and `inTexCoord` (location 1, vec2) match the `MeshPU` vertex layout. These come from the vertex buffer.
- **Camera UBO (set 0, binding 0)** -- a single `mat4 projectionView`. Declared as a `uniform` block, it reads from the buffer bound to descriptor set 0.
- **Model UBO (set 1, binding 0)** -- a single `mat4 model`. Declared as a `uniform` block, it reads from the buffer bound to descriptor set 1.
- **Transformation chain** -- `gl_Position = projectionView * model * position`. The camera matrix is applied first (world to clip space), then the model matrix (local to world space). This is the standard MVP transformation with view and projection pre-combined.
- **Texture coordinate passthrough** -- `inTexCoord` is passed to the fragment shader as `fragUV` via rasterization interpolation.

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

Key concepts:

- **Separate image and sampler (set 2, binding 0 and 1)** -- this uses the Vulkan `GL_EXT_nonuniform_qualifier` pattern where the texture and sampler are separate resources. `texture2D uTex` is the image, `sampler uSampler` is the sampling state. They are combined at runtime with `sampler2D(uTex, uSampler)`.
- **Why separate?** This allows mixing and matching textures with different samplers (e.g., nearest vs. linear filtering) without duplicating texture resources. It is the standard pattern in modern Vulkan/Metal rendering.
- **`sampler2D(uTex, uSampler)`** -- combines the image and sampler into a sampled image for the `texture()` call.
- **Output** -- the sampled color is written directly to the framebuffer. No lighting or additional processing is applied.

## Key concepts

### Descriptor set organization

This example uses three descriptor sets to organize resources by update frequency:

```
Set 0 (Camera)      ─── CameraUBO { projectionView }     [never changes]
Set 1 (Model)       ─── ModelUBO { model }                [updated every frame]
Set 2 (Material)    ─── texture2D + sampler               [never changes]
```

This is the standard pattern for real-time rendering:

| Set | Contents | Update frequency | Example |
|-----|----------|-----------------|---------|
| 0 | View/projection | Once per camera move | Scene-level data |
| 1 | Object transform | Once per object per frame | Per-object data |
| 2 | Material textures | Once per material change | Material data |

Separating into multiple sets allows the GPU to cache bindings efficiently. If only set 1 changes between draw calls, sets 0 and 2 remain bound.

**Reference:** [ResourceSet API -- multi-set layout](../../api/gpu/ResourceSet.md#usage-pattern)

### FrameResourceRing for per-frame data

The `FrameResourceRing<T>` template solves the CPU-GPU synchronization problem for frequently updated resources:

```cpp
gpu::FrameResourceRing<gpu::Buffer> ring;
ring.create(surface.get(), [&](uint32_t i) {
    // Create one resource per in-flight frame
    return device->createBuffer("Ring[" + std::to_string(i) + "]");
});

// Each frame:
auto* res = ring.current();  // Get current frame's resource
res->updateUniformBuffer(newData);  // Write new data (host-visible, no sync)
```

The ring size matches `surface->inFlightFrames()` (typically 2 for windowed rendering). On frame N, the CPU writes to slot N % 2, and the GPU reads from slot (N-1) % 2. Since the CPU only writes to slots the GPU is done reading, no synchronization is needed.

**Reference:** [FrameResourceRing API](../../api/gpu/FrameResourceRing.md)

### Host-visible vs. device-local buffers

The `Buffer` class supports two allocation strategies:

| Method | Memory type | Use case | Sync |
|--------|------------|----------|------|
| `createVertexBuffer` | Device-local | Static geometry | Staging upload |
| `createUniformBuffer` | Host-visible | Per-frame data | Direct CPU write |
| `createStorageBuffer` | Host-visible | Large per-frame data | Direct CPU write |

Uniform buffers use host-visible memory so `updateUniformBuffer` can `memcpy` directly into the buffer without staging or GPU synchronization. This is the fastest path for per-frame updates.

**Reference:** [Buffer API](../../api/gpu/Buffer.md)

### Image layout transitions

Every image operation requires a specific layout. This example uses three layouts for the color image:

| Layout | Used by |
|--------|---------|
| `ColorAttachment` | Graphics fragment shader output |
| `Present` | Swapchain presentation |
| `ShaderReadOnly` | Fragment shader texture sampling |

The texture is transitioned to `ShaderReadOnly` once after upload. The frame color image transitions from `ColorAttachment` (rendering) to `Present` (display) each frame.

**Reference:** [CommandBuffer API -- transition()](../../api/gpu/CommandBuffer.md)

### Depth testing

Setting `depthFormat` in the pipeline description enables depth testing with `CompareFunctionLess` and depth writes enabled. The depth buffer ensures that closer fragments occlude farther fragments, which is essential for correct rendering of 3D geometry like a cube.

Each frame clears the depth buffer to 1.0 (far plane) before rendering.

**Reference:** [GraphicsPipeline API -- depth testing](../../api/gpu/GraphicsPipeline.md#depth-testing)

## Building and running

Build the example with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_uniform_buffers
```

Then run the binary from the `bin/` directory. On macOS you will be prompted to choose a backend. You should see a window with a rotating textured cube on a dark background.

## Next steps

- **[08_render_to_texture](08_render_to_texture.md)** -- Render to an offscreen texture and use it as input.
- **[09_multiple_objects](09_multiple_objects.md)** -- Draw multiple objects with per-object transforms.
- **[GPU API -- Buffer](../../api/gpu/Buffer.md)** -- Full reference for uniform and storage buffers.
- **[GPU API -- FrameResourceRing](../../api/gpu/FrameResourceRing.md)** -- Full reference for per-frame resource management.
- **[GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)** -- Full reference for descriptor set binding.
