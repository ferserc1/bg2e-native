# Tutorial 09: Cubemap Rendering and Mirror Reflection

This tutorial walks through the `09_cubemap` example: loading an equirectangular HDR texture, rendering it into a six-face cubemap, then using that cubemap to render a mirror cube with per-pixel reflection. This is the foundational example for environment mapping using the GPU abstraction layer.

**Source:** `examples/gpu/09_cubemap/src/main.cpp`

## What you will learn

- How to create a `gpu::CubeMap` render target with six faces
- How to use `beginRendering(CubeMap*, face, mipLevel)` to render into individual cubemap faces
- How to load and upload an equirectangular HDR image as a `gpu::Image`
- How to bind cubemap textures to shaders via `setSampledCubeMap()`
- How to use `textureCube` and `samplerCube` in GLSL fragment shaders
- How to implement per-pixel reflection using `reflect()` and the camera position
- How to manage multiple `FrameResourceRing` objects for different objects
- How to use push constants for per-draw camera data
- How to chain two render passes in a single command buffer: cubemap pass and main pass

## Prerequisites

- Completed [08_render_to_texture](08_render_to_texture.md) -- you should understand offscreen rendering, image layout transitions, and multi-pass pipelines
- bg2e-native built and available on your system
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding cubemap rendering

A cubemap is a texture composed of six square faces, one for each direction along the Cartesian axes (+X, -X, +Y, -Y, +Z, -Z). Cubemaps are used for environment mapping -- storing the surrounding environment from the perspective of a point in space, then sampling that environment by direction for reflections, refractions, and skyboxes.

This example implements a two-pass pipeline:

```
Pass 1: Cubemap Pass         Pass 2: Main Pass
 ┌──────────────────┐       ┌────────────────────┐
 │ Render sphere    │       │ Render mirror cube  │
 │ into 6 cubemap   │──────>│ on screen using     │──> Surface
 │ faces            │       │ the cubemap         │
 │                  │       │                     │
 └──────────────────┘       └────────────────────┘
```

The cubemap pass renders an equirectangular HDR environment texture into a cube-shaped render target by projecting it onto a sphere from six different viewpoints. The main pass then samples this cubemap to compute mirror-like reflections on a rotating cube.

| Pass | Render target | Pipeline | Key operation |
|------|--------------|----------|---------------|
| 1 | CubeMap (6 faces) | Cubemap renderer | Equirect texture -> cubemap faces |
| 2 | Surface (swapchain) | Cube reflection | Cubemap sampling with reflection |

**Reference:** [GPU API -- Quick Start: Recipe 9](../../api/gpu/quick_start.md#recipe-9-cubemap-rendering)

## Step-by-step code explanation

### 1. Data structures for push constants and UBOs

```cpp
struct CubemapCameraPushConstants {
    glm::mat4 projection;
    glm::mat4 view;
};

struct CubeReflectionPushConstants {
    glm::vec3 cameraPos;
    float padding;
};

struct ModelUBO {
    glm::mat4 model;
};

struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 cameraPos;
};

struct CubeRenderSettingsUBO {
    uint32_t mode; // 0 = reflection, 1 = direct cubemap debug
    uint32_t _pad0 = 0;
    uint32_t _pad1 = 0;
    uint32_t _pad2 = 0;
};
```

This example defines two push constant structs and three UBO structs:

- **`CubemapCameraPushConstants`**: projection and view matrices for each cubemap face, passed via push constants to the cubemap renderer vertex shader
- **`CubeReflectionPushConstants`**: camera world position, passed via push constants to the reflection fragment shader for computing reflection vectors
- **`ModelUBO`**: model matrix, updated per-frame for each object (sphere and cube)
- **`CameraUBO`**: persistent camera data with projection, view, and camera position
- **`CubeRenderSettingsUBO`**: per-frame settings controlling the rendering mode (reflection vs. debug cubemap visualization)

The `CubeRenderSettingsUBO` uses `uint32_t` fields with explicit padding to ensure the struct is 16-byte aligned, which is required for Vulkan uniform buffer compatibility.

### 2. Two pipeline layouts: cubemap renderer and cube reflection

```cpp
// Cubemap renderer pipeline layout
gpu::PipelineLayoutDescription cubemapRendererLayoutDesc{};
cubemapRendererLayoutDesc.pushConstants.push_back(
    {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
);
cubemapRendererLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Vertex, 1
});
cubemapRendererLayoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage,
    gpu::ShaderStage::Fragment, 1
});
cubemapRendererLayoutDesc.resourceBindings.push_back({
    1, {.vulkan = 1, .metal = 0},
    gpu::ResourceType::Sampler,
    gpu::ShaderStage::Fragment, 1
});
cubemapRendererLayoutDesc.debugName = "Cubemap renderer pipeline layout";
auto cubemapRendererLayout = device->createPipelineLayout(cubemapRendererLayoutDesc);
```

The cubemap renderer layout has two descriptor sets:

| Set | Binding | Type | Stage | Purpose |
|-----|---------|------|-------|---------|
| 0 | 0 | UniformBuffer | Vertex | Model UBO (identity matrix for the sphere) |
| 1 | 0 | SampledImage | Fragment | Equirectangular HDR texture |
| 1 | 1 | Sampler | Fragment | Linear sampler for texture sampling |

Plus push constants in the vertex stage carrying the per-face projection and view matrices. This layout is simple because the cubemap renderer only needs to project the equirect texture onto a sphere for each face.

```cpp
// Cube reflection pipeline layout
gpu::PipelineLayoutDescription cubeReflectionLayoutDesc{};
cubeReflectionLayoutDesc.pushConstants.push_back(
    {0, sizeof(CubeReflectionPushConstants), gpu::ShaderStage::Fragment}
);
cubeReflectionLayoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Vertex, 1
});
cubeReflectionLayoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Vertex, 1
});
cubeReflectionLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 0},
    gpu::ResourceType::SampledImage,
    gpu::ShaderStage::Fragment, 1
});
cubeReflectionLayoutDesc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 0},
    gpu::ResourceType::Sampler,
    gpu::ShaderStage::Fragment, 1
});
cubeReflectionLayoutDesc.resourceBindings.push_back({
    3, {.vulkan = 0, .metal = 3},
    gpu::ResourceType::UniformBuffer,
    gpu::ShaderStage::Fragment, 1
});
cubeReflectionLayoutDesc.debugName = "Cube reflection pipeline layout";
auto cubeReflectionLayout = device->createPipelineLayout(cubeReflectionLayoutDesc);
```

The cube reflection layout has four descriptor sets:

| Set | Binding | Type | Stage | Purpose |
|-----|---------|------|-------|---------|
| 0 | 0 | UniformBuffer | Vertex | Camera UBO (projection, view, cameraPos) |
| 1 | 0 | UniformBuffer | Vertex | Model UBO (cube rotation matrix) |
| 2 | 0 | SampledImage | Fragment | Cubemap texture |
| 2 | 1 | Sampler | Fragment | Cubemap sampler |
| 3 | 0 | UniformBuffer | Fragment | Render settings (mode flag) |

Plus push constants in the **fragment** stage carrying the camera world position. This separation keeps the camera data persistent in the UBO while the per-draw camera position is pushed for reflection computation.

**Reference:** [GPU API -- PipelineLayout](../../api/gpu/PipelineLayout.md), [GPU API -- ResourceSet: Metal binding rules](../../api/gpu/ResourceSet.md#metal-binding-rules)

### 3. Equirect texture loading and cubemap creation

```cpp
auto assetsPath = base::PlatformTools::assetPath();
auto imageData = std::shared_ptr<base::Image>(
    bg2e::db::loadImage(assetsPath / "autumn_field_4k.hdr")
);
auto equirectTexture = device->createImage({
    .size = {imageData->width(), imageData->height()},
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
    .debugName = "Equirect HDR texture"
});
equirectTexture->uploadImage(imageData.get());

device->immediateSubmit([equirectTexture](gpu::CommandBuffer* cmd) {
    cmd->transition(equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
});
```

The equirectangular HDR image is loaded using `bg2e::db::loadImage()`, which returns a `base::Image` with the pixel data. The image is then uploaded to a `gpu::Image` with:

- **Format**: `R16G16B16A16_SFLOAT` -- 16-bit float per channel, required for HDR data that exceeds the [0,1] range
- **Usage**: `Sampled | TransferDst` -- the image is sampled in the fragment shader and needs transfer destination for the upload
- **Layout transition**: after upload, the image transitions to `ShaderReadOnly` so it can be sampled by the cubemap renderer

```cpp
auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
```

A default linear sampler provides bilinear filtering for smooth texture sampling.

```cpp
auto equirectTextureSet = device->createResourceSet(
    cubemapRendererLayout.get(), 1, "Equirect texture set"
);
equirectTextureSet->setSampledImage({.vulkan = 0, .metal = 0}, equirectTexture.get());
equirectTextureSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
equirectTextureSet->update();
```

The equirect texture set is bound to descriptor set 1 of the cubemap renderer layout. This is a **persistent** set -- it is created once and reused every frame without re-updating.

```cpp
const uint32_t cubemapSize = 512;
auto cubeMap = device->createCubeMap({
    .size = cubemapSize,
    .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
    .usage = gpu::ImageUsage::Sampled
           | gpu::ImageUsage::ColorAttachment
           | gpu::ImageUsage::TransferSrc
           | gpu::ImageUsage::TransferDst,
    .mipLevels = 1,
    .debugName = "Skybox cubemap"
});
```

The cubemap is created with:

- **Size**: 512x512 per face
- **Format**: `R16G16B16A16_SFLOAT` -- matches the equirect texture format for HDR accuracy
- **Usage flags**:
  - `Sampled` -- the cubemap is sampled as a `textureCube` in the reflection shader
  - `ColorAttachment` -- each face is rendered into as a color attachment
  - `TransferSrc | TransferDst` -- allows copying between cubemap faces if needed
- **Mip levels**: 1 -- no mipmaps (could be increased for trilinear filtering at distance)

```cpp
auto cubemapTextureSet = device->createResourceSet(
    cubeReflectionLayout.get(), 2, "Cubemap texture set"
);
cubemapTextureSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, cubeMap.get());
cubemapTextureSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
cubemapTextureSet->update();
```

The cubemap is bound to descriptor set 2 of the reflection layout using `setSampledCubeMap()`. This method extracts the underlying `gpu::Image*` from the `CubeMap` and binds it as a sampled image, which the GLSL shader accesses via `textureCube`.

**Reference:** [GPU API -- Quick Start: Cubemap creation](../../api/gpu/quick_start.md#recipe-9-cubemap-rendering), [GPU API -- ResourceSet::setSampledCubeMap()](../../api/gpu/ResourceSet.md#void-setsampledcubemap)

### 4. Camera UBO (persistent)

```cpp
const float aspect = 800.0f / 600.0f;
glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
glm::vec3 cameraEye(0.0f, 0.0f, 3.0f);
glm::mat4 view = glm::lookAt(
    cameraEye,
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

CameraUBO cameraData{};
cameraData.projection = projection;
cameraData.view = view;
cameraData.cameraPos = glm::vec4(cameraEye, 1.0f);

auto cameraUbo = device->createBuffer("Camera UBO");
cameraUbo->createUniformBuffer(cameraData);

auto cameraSet = device->createResourceSet(cubeReflectionLayout.get(), 0, "Camera resource set");
cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
cameraSet->update();
```

The camera UBO is **persistent** -- it is created once with the initial view/projection matrices and camera position. The camera does not move in this example, so the UBO is never updated after creation. This is bound to descriptor set 0 of the reflection layout.

Note the `cameraPos` field is stored as a `glm::vec4` (with `.w = 1.0`) to match the GLSL `vec4 cameraPos` in the shader. The shader uses only the `.xyz` components.

### 5. Multiple FrameResourceRings for different objects

This example uses four `FrameResourceRing` objects -- two for the cubemap renderer sphere and two for the reflection cube:

```cpp
// Cubemap renderer sphere rings
gpu::FrameResourceRing<gpu::Buffer> cubemapModelUboRing;
cubemapModelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer(
        "Cubemap model UBO ring[" + std::to_string(i) + "]"
    );
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> cubemapModelSetRing;
cubemapModelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(
        cubemapRendererLayout.get(), 0,
        "Cubemap model set ring[" + std::to_string(i) + "]"
    );
    set->setUniformBuffer({.vulkan = 0, .metal = 2}, cubemapModelUboRing.sharedAt(i));
    set->update();
    return set;
});
```

The cubemap renderer uses an identity model matrix (the sphere is at the origin), but the `FrameResourceRing` is still needed because the push constants change per face and the model UBO must be available for the current frame slot.

```cpp
// Cube reflection rings
gpu::FrameResourceRing<gpu::Buffer> cubeModelUboRing;
cubeModelUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer(
        "Cube model UBO ring[" + std::to_string(i) + "]"
    );
    buffer->createUniformBuffer(ModelUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> cubeModelSetRing;
cubeModelSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(
        cubeReflectionLayout.get(), 1,
        "Cube model set ring[" + std::to_string(i) + "]"
    );
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, cubeModelUboRing.sharedAt(i));
    set->update();
    return set;
});

// Cube settings rings
gpu::FrameResourceRing<gpu::Buffer> cubeSettingsUboRing;
cubeSettingsUboRing.create(surface.get(), [&](uint32_t i) {
    auto buffer = device->createBuffer(
        "Cube settings UBO ring[" + std::to_string(i) + "]"
    );
    buffer->createUniformBuffer(CubeRenderSettingsUBO{});
    return buffer;
});

gpu::FrameResourceRing<gpu::ResourceSet> cubeSettingsSetRing;
cubeSettingsSetRing.create(surface.get(), [&](uint32_t i) {
    auto set = device->createResourceSet(
        cubeReflectionLayout.get(), 3,
        "Cube settings set ring[" + std::to_string(i) + "]"
    );
    set->setUniformBuffer({.vulkan = 0, .metal = 3}, cubeSettingsUboRing.sharedAt(i));
    set->update();
    return set;
});
```

The cube reflection pipeline uses two additional rings:

- **Model UBO ring** (set 1): updated every frame with the rotating cube model matrix
- **Settings UBO ring** (set 3): updated every frame with the render mode flag

Each ring creates `inFlightFrames()` slots (typically 2 for window surfaces). The `current()` method returns the resource for the current frame, and `sharedAt(i)` provides a `shared_ptr` for deferred cleanup or resource set binding.

**Reference:** [GPU API -- FrameResourceRing](../../api/gpu/FrameResourceRing.md)

### 6. Sphere and cube meshes

```cpp
// Sphere for cubemap rendering (position + texCoord0)
std::unique_ptr<bg2e::geo::MeshPU> sphereData(
    bg2e::geo::createSpherePU(50.0f, 16, 16, true)
);
gpu::MeshPU cubemapSphere;
cubemapSphere.setMeshData(*sphereData);
cubemapSphere.build(device.get());

// Cube for mirror reflection (position + normal + texCoord0)
std::unique_ptr<bg2e::geo::MeshPNU> cubeData(
    bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f)
);
gpu::MeshPNU cube;
cube.setMeshData(*cubeData);
cube.build(device.get());
```

Two different mesh types are used:

- **`MeshPU`** (position + texCoord0): the sphere only needs position and texture coordinates. The cubemap renderer vertex shader transforms vertices with push-constant MVP matrices and passes UV coordinates to sample the equirect texture. The sphere is large (radius 50) and rendered from inside (`true` parameter enables inside-facing normals) so it fills the entire cubemap face.

- **`MeshPNU`** (position + normal + texCoord0): the reflection cube needs normals for computing reflection vectors. The vertex shader outputs world-space position, normal, and local position for the fragment shader.

### 7. Cubemap face camera setup

```cpp
const float nearPlane = 0.1f;
const float farPlane = 1000.0f;
auto cubemapProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

struct CubemapFaceCamera {
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;
};

std::array<CubemapFaceCamera, 6> faceCameras = {{
    { glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +X
    { glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -X
    { glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f) }, // +Y
    { glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f) }, // -Y
    { glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +Z
    { glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -Z
}};
```

Each cubemap face requires a 90-degree field-of-view camera looking along one axis from the origin. The projection uses a 1:1 aspect ratio (square faces) and 90-degree FOV. The `up` vectors are chosen to match the Vulkan/Metal coordinate conventions.

The six cameras correspond to:

| Face | Direction | Up vector | What it sees |
|------|-----------|-----------|--------------|
| +X | Right | Down | Right hemisphere |
| -X | Left | Down | Left hemisphere |
| +Y | Up | Forward | Upper hemisphere |
| -Y | Down | Backward | Lower hemisphere |
| +Z | Forward | Down | Front hemisphere |
| -Z | Backward | Down | Back hemisphere |

### 8. Graphics pipelines

```cpp
auto colorFormat = surface->colorFormat();
auto depthFormat = surface->depthFormat();

// Cubemap renderer pipeline
gpu::GraphicsPipelineDescription cubemapRendererPipelineDesc{};
cubemapRendererPipelineDesc.vertexShader   = cubemapRendererVs.get();
cubemapRendererPipelineDesc.fragmentShader = cubemapRendererFs.get();
cubemapRendererPipelineDesc.layout         = cubemapRendererLayout.get();
cubemapRendererPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
cubemapRendererPipelineDesc.colorFormat    = gpu::PixelFormat::R16G16B16A16_SFLOAT;
cubemapRendererPipelineDesc.depthFormat    = gpu::PixelFormat::Undefined;
cubemapRendererPipelineDesc.cullMode       = gpu::CullMode::None;
cubemapRendererPipelineDesc.debugName      = "Cubemap renderer pipeline";
cubemapRendererPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto cubemapRendererPipeline = device->createGraphicsPipeline(cubemapRendererPipelineDesc);
```

The cubemap renderer pipeline has two critical differences from a standard pipeline:

- **`colorFormat = R16G16B16A16_SFLOAT`**: matches the cubemap's pixel format, not the swapchain format. The pipeline renders into cubemap faces, not the screen.
- **`depthFormat = Undefined`**: no depth buffer is needed for cubemap rendering since each face is rendered independently.
- **`cullMode = CullMode::None`**: the sphere is rendered from inside, so face culling must be disabled.

```cpp
// Cube reflection pipeline
gpu::GraphicsPipelineDescription cubeReflectionPipelineDesc{};
cubeReflectionPipelineDesc.vertexShader   = cubeReflectionVs.get();
cubeReflectionPipelineDesc.fragmentShader = cubeReflectionFs.get();
cubeReflectionPipelineDesc.layout         = cubeReflectionLayout.get();
cubeReflectionPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
cubeReflectionPipelineDesc.colorFormat    = colorFormat;
cubeReflectionPipelineDesc.depthFormat    = depthFormat;
cubeReflectionPipelineDesc.debugName      = "Cube reflection pipeline";
cubeReflectionPipelineDesc.addVertexBufferDescription(gpu::MeshPNU::vertexBufferDescription());
auto cubeReflectionPipeline = device->createGraphicsPipeline(cubeReflectionPipelineDesc);
```

The reflection pipeline uses the surface formats and includes depth testing for proper 3D rendering.

**Reference:** [GPU API -- GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 9. Render loop with two passes

```cpp
auto& graphicsQueue = device->graphicsQueue();

bool running = true;
bool debugCubemapMode = false;
while (running)
{
    // ... event handling, window resize, space bar toggle ...

    const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    cmd->begin();
```

The render loop begins with the standard frame acquisition and command buffer creation. The `debugCubemapMode` flag toggles between reflection mode (mode 0) and debug cubemap visualization (mode 1) via the space bar.

#### Cubemap pass: render sphere into 6 cubemap faces

```cpp
    // --- Cubemap pass: render sphere into 6 cubemap faces ---
    ModelUBO identityModel{};
    identityModel.model = glm::mat4(1.0f);

    auto* cubemapModelUbo = cubemapModelUboRing.current();
    cubemapModelUbo->updateUniformBuffer(identityModel);
    auto* cubemapModelSet = cubemapModelSetRing.current();

    for (uint32_t face = 0; face < 6; ++face)
    {
        auto& fc = faceCameras[face];
        glm::mat4 faceView = glm::lookAt(fc.eye, fc.center, fc.up);

        CubemapCameraPushConstants cameraPushData{};
        cameraPushData.projection = cubemapProj;
        cameraPushData.view = faceView;

        cmd->transition(cubeMap->image(), gpu::ImageLayout::ColorAttachment);
        cmd->beginRendering(cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
        cmd->clearColor(0, gpu::Color(0.0f, 0.0f, 0.0f, 1.0f));

        cmd->bindPipeline(cubemapRendererPipeline.get());
        cmd->bindResourceSet(cubemapRendererPipeline.get(), 0, cubemapModelSet);
        cmd->bindResourceSet(cubemapRendererPipeline.get(), 1, equirectTextureSet.get());
        cmd->pushConstants(gpu::ShaderStage::Vertex, 0,
            sizeof(CubemapCameraPushConstants), &cameraPushData);
        cubemapSphere.draw(cmd.get());

        cmd->endRendering();
    }

    // Transition cubemap to shader readable
    cmd->transition(cubeMap->image(), gpu::ImageLayout::ShaderReadOnly);
```

This is the key section of the example. For each of the 6 cubemap faces:

1. **Transition** the cubemap image to `ColorAttachment` layout
2. **Begin rendering** into the specific face using `beginRendering(cubeMap.get(), face, mipLevel)`
3. **Clear** the face to black
4. **Bind** the cubemap renderer pipeline and resource sets
5. **Push constants** with the face-specific projection and view matrices
6. **Draw** the sphere (which projects the equirect texture onto the face)
7. **End rendering**

After all 6 faces are rendered, the cubemap transitions to `ShaderReadOnly` so it can be sampled by the reflection shader.

The `beginRendering(CubeMap*, face, mipLevel)` overload is the cubemap-specific rendering entry point. It renders into a specific face of the cubemap at a given mip level.

**Reference:** [GPU API -- CommandBuffer::beginRendering()](../../api/gpu/CommandBuffer.md)

#### Main pass: render mirror cube on screen

```cpp
    // --- Main pass: render mirror cube on screen using the cubemap ---
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
    cmd->clearDepth(1.0f);

    {
        ModelUBO cubeModelData{};
        cubeModelData.model = glm::rotate(
            glm::mat4(1.0f), t,
            glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f))
        );

        auto* cubeModelUbo = cubeModelUboRing.current();
        cubeModelUbo->updateUniformBuffer(cubeModelData);
        auto* cubeModelSet = cubeModelSetRing.current();

        CubeRenderSettingsUBO settings{};
        settings.mode = debugCubemapMode ? 1u : 0u;
        auto* settingsUbo = cubeSettingsUboRing.current();
        settingsUbo->updateUniformBuffer(settings);
        auto* settingsSet = cubeSettingsSetRing.current();

        CubeReflectionPushConstants reflectionPushData{};
        reflectionPushData.cameraPos = cameraEye;

        cmd->bindPipeline(cubeReflectionPipeline.get());
        cmd->pushConstants(gpu::ShaderStage::Fragment, 0,
            sizeof(CubeReflectionPushConstants), &reflectionPushData);
        cmd->bindResourceSet(cubeReflectionPipeline.get(), 0, cameraSet.get());
        cmd->bindResourceSet(cubeReflectionPipeline.get(), 1, cubeModelSet);
        cmd->bindResourceSet(cubeReflectionPipeline.get(), 2, cubemapTextureSet.get());
        cmd->bindResourceSet(cubeReflectionPipeline.get(), 3, settingsSet);

        cube.draw(cmd.get());
    }

    cmd->endRendering();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());
}
```

The main pass renders the mirror cube:

1. **Update per-frame UBOs**: the cube's model matrix rotates over time, and the settings UBO controls the rendering mode
2. **Push constants**: the camera world position is pushed to the fragment shader for reflection vector computation
3. **Bind four resource sets**: camera (set 0), model (set 1), cubemap (set 2), settings (set 3)
4. **Draw** the cube: `MeshPNU::draw()` binds vertex/index buffers and issues indexed draw calls

The cube rotates around an arbitrary axis `(4, 1, 0.5)` normalized, giving a visually interesting spin that showcases the reflection from multiple angles.

### 10. Cleanup

```cpp
device->waitIdle();

cubemapModelSetRing.cleanup();
cubemapModelUboRing.cleanup();
cubemapSphere.cleanup();

cubeSettingsSetRing.cleanup();
cubeSettingsUboRing.cleanup();

cubeModelUboRing.cleanup();
cubeModelSetRing.cleanup();
cube.cleanup();

cleanup.flush();

surface->cleanup();
device->cleanup();
instance->cleanup();
SDL_DestroyWindow(window);
SDL_Quit();
```

Cleanup follows the standard reverse-order pattern: `FrameResourceRing` objects first (they hold `shared_ptr` references that may point to `CleanupManager`-owned resources), then `CleanupManager::flush()` for everything else, then surface/device/instance teardown.

## Shader code explanation

### Cubemap Renderer Vertex Shader (`cubemap_renderer.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform CameraData {
    mat4 projection;
    mat4 view;
} camera;

layout(set = 0, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    gl_Position = camera.projection * camera.view * object.model * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}
```

This vertex shader transforms the sphere vertices using the push-constant projection/view matrices and the model UBO matrix. The texture coordinates are passed through to the fragment shader. Since the sphere is large and rendered from inside, the projected positions cover the entire cubemap face.

### Cubemap Renderer Fragment Shader (`cubemap_renderer.frag.glsl`)

```glsl
#version 450

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform sampler   uSampler;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(uTex, uSampler), fragUV);
}
```

The fragment shader samples the equirectangular HDR texture using the interpolated UV coordinates. The `texture2D` combined sampler is constructed from the separate texture and sampler objects using `sampler2D(uTex, uSampler)`. This is the standard Vulkan/Metal pattern for separate image samplers.

### Cube Reflection Vertex Shader (`reflect_cube.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragLocalPos;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
} camera;

layout(set = 1, binding = 0) uniform ModelUBO {
    mat4 model;
} object;

void main()
{
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    gl_Position = camera.projection * camera.view * worldPos;
    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(object.model) * inNormal;
    fragLocalPos = inPosition;
}
```

The vertex shader outputs three interpolated values:

- **`fragWorldPos`**: world-space position of the fragment (for computing the reflection vector)
- **`fragNormal`**: world-space normal (transformed by the model matrix's upper-left 3x3)
- **`fragLocalPos`**: local-space position (for debug cubemap visualization mode)

The normal is transformed using `mat3(object.model)` which extracts the rotation/scale part of the model matrix. For uniform scaling this works correctly; for non-uniform scaling, the normal matrix `(M^-1)^T` would be needed.

### Cube Reflection Fragment Shader (`reflect_cube.frag.glsl`)

```glsl
#version 450

layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler   uSampler;

layout(set = 3, binding = 0) uniform CubeRenderSettings {
    uint mode;
    uint _pad0;
    uint _pad1;
    uint _pad2;
} settings;

layout(push_constant) uniform PushConstants {
    vec3 cameraPos;
} push;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

void main()
{
    if (settings.mode == 0) {
        vec3 N = normalize(fragNormal);
        vec3 I = normalize(fragWorldPos - push.cameraPos);
        vec3 R = reflect(I, N);
        outColor = texture(samplerCube(uCubeMap, uSampler), R);
    }
    else {
        vec3 D = normalize(fragLocalPos);
        outColor = texture(samplerCube(uCubeMap, uSampler), D);
    }
}
```

This is the core of the reflection effect. The fragment shader has two rendering modes:

**Mode 0 -- Mirror reflection:**

1. **`N = normalize(fragNormal)`**: normalize the interpolated surface normal
2. **`I = normalize(fragWorldPos - push.cameraPos)`**: compute the incident vector from camera to fragment
3. **`R = reflect(I, N)`**: compute the reflection vector using GLSL's built-in `reflect()` function, which computes `I - 2 * dot(N, I) * N`
4. **`texture(samplerCube(uCubeMap, uSampler), R)`**: sample the cubemap using the reflection direction

This produces a perfect mirror reflection -- the cubemap is sampled in the direction that a light ray would reflect off the surface toward the camera.

**Mode 1 -- Debug cubemap visualization:**

Uses the local-space position as the cubemap sampling direction. This shows the cubemap as if it were projected onto the cube's interior, useful for verifying that the cubemap was rendered correctly.

The `textureCube()` function takes a 3D direction vector and samples the cubemap face that the direction points toward. The `samplerCube()` combines the cubemap texture with a sampler for filtering.

## Key concepts

### Cubemap rendering with `beginRendering(CubeMap*, face, mipLevel)`

The `CommandBuffer` provides a cubemap-specific `beginRendering` overload:

```cpp
cmd->beginRendering(cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
```

This renders into a specific face of the cubemap. The `CubemapFace` enum values (0-5) correspond to +X, -X, +Y, -Y, +Z, -Z. The mip level parameter allows rendering into specific mip levels for mipmap chain generation.

The cubemap image must be transitioned to `ColorAttachment` before rendering and to `ShaderReadOnly` after all faces are rendered.

**Reference:** [GPU API -- Quick Start: Render into cubemap faces](../../api/gpu/quick_start.md#render-into-cubemap-faces)

### Separate image samplers in GLSL

The shaders use the Vulkan/Metal pattern of separate image and sampler objects:

```glsl
layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform sampler   uSampler;
// ...
outColor = texture(sampler2D(uTex, uSampler), fragUV);
```

This is different from the combined `sampler2D` used in OpenGL. The separate approach allows:
- Sharing samplers across multiple textures
- Independent lifetime management of textures and samplers
- Better GPU cache utilization

For cubemaps, the same pattern applies:

```glsl
layout(set = 2, binding = 0) uniform textureCube uCubeMap;
layout(set = 2, binding = 1) uniform sampler   uSampler;
// ...
outColor = texture(samplerCube(uCubeMap, uSampler), R);
```

### Push constants vs. uniform buffers

This example uses both push constants and uniform buffers strategically:

| Data | Storage | Why |
|------|---------|-----|
| Per-face MVP matrices | Push constants | Changes 6 times per frame, small size (128 bytes) |
| Camera position | Push constants | Changes per draw call, small size (16 bytes) |
| Model matrix | Uniform buffer (ring) | Changes per object per frame |
| Camera projection/view | Uniform buffer (persistent) | Never changes |
| Render settings | Uniform buffer (ring) | Changes per frame |

Push constants are faster for small, frequently-changing data because they are embedded directly in the command buffer. Uniform buffers are better for larger or persistently-bound data.

### Two-pass rendering in a single command buffer

Both passes are recorded into the same command buffer:

```
cmd->begin()
  [Cubemap pass: 6 face renders]
  cmd->transition(cubeMap, ShaderReadOnly)
  [Main pass: 1 screen render]
  cmd->transition(color, Present)
cmd->end()
```

The GPU executes these passes sequentially within the command buffer. The cubemap transition between passes ensures the cubemap data is visible to the main pass shader.

### Metal binding index considerations

The pipeline layouts use different Metal buffer indices for each set to avoid collisions:

| Set | Resource | Metal buffer index |
|-----|----------|-------------------|
| 0 | Camera UBO (vertex) | 2 (0 = vertex buffer, 1 = push constants) |
| 1 | Model UBO (vertex) | 3 |
| 2 | Cubemap texture | texture(0) |
| 3 | Settings UBO (fragment) | 3 (0 = push constants, 1+ available) |

The Metal binding rules require buffer indices >= 2 in the vertex stage and >= 1 in the fragment stage to avoid reserved slots.

**Reference:** [GPU API -- PipelineLayout: Metal buffer index reservation](../../api/gpu/PipelineLayout.md#metal-buffern-index-reservation)

## Building and running

Build the example with CMake:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target gpu_cubemap
```

Then run the binary from the `bin/` directory. On macOS you will be prompted to choose a backend (Metal or Vulkan). You should see a window with a rotating cube that reflects the HDR environment map. Press the space bar to toggle between reflection mode and debug cubemap visualization.

## Next steps

- **[10_cubemap_render_pass](10_cubemap_render_pass.md)** -- Cubemap rendering with a dedicated render pass.
- **[GPU API -- Quick Start: Recipe 9](../../api/gpu/quick_start.md#recipe-9-cubemap-rendering)** -- Full reference for cubemap rendering.
- **[GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)** -- Full reference for descriptor set binding and `setSampledCubeMap()`.
- **[GPU API -- FrameResourceRing](../../api/gpu/FrameResourceRing.md)** -- Full reference for per-frame resource management.
- **[GPU API -- CommandBuffer](../../api/gpu/CommandBuffer.md)** -- Full reference for `beginRendering(CubeMap*, face, mipLevel)`.
