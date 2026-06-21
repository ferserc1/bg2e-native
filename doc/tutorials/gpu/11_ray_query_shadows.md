# Tutorial 11: Ray Query Shadows

This tutorial walks through the `11_ray_query_shadows` example: using hardware ray queries from a fragment shader to compute hard shadows in a Lambert-lit scene. The example renders a ground plane, cube, and sphere with an orbiting point light, casting visibility rays from each fragment toward the light to determine shadow occlusion.

**Source:** `examples/gpu/11_ray_query_shadows/src/main.cpp`

## What you will learn

- How to detect hardware ray tracing support on the physical device
- How to create `RayTracingMesh` objects (bottom-level acceleration structures) from existing rasterization meshes
- How to build BLAS objects using `immediateSubmit`
- How to create and populate a `RayTracingScene` (top-level acceleration structure) with instances
- How to bind an acceleration structure to a shader via `ResourceSet::setRayTracingScene()`
- How to rebuild the TLAS dynamically each frame
- How to use `GL_EXT_ray_query` in GLSL to cast visibility rays for shadow testing
- How to configure a pipeline layout with `ResourceType::AccelerationStructure`

## Prerequisites

- Completed [09_cubemap](09_cubemap.md) -- you should understand resource sets, UBOs, and multi-set pipeline layouts
- bg2e-native built with ray tracing support (Vulkan SDK 1.3+ or Metal on macOS)
- GLSL shaders compiled (the build system compiles `.glsl` to `.spv` automatically)

## Understanding ray queries and shadows

Ray queries allow fragment shaders (or any non-ray-tracing shader) to cast rays into a scene and query intersection results without using a full ray tracing pipeline. This is useful for effects like:

- **Hard shadows**: cast a ray from the fragment toward a light source; if it hits geometry, the fragment is in shadow
- **Ambient occlusion**: cast rays in multiple directions to approximate local occlusion
- **Reflections**: cast rays in the reflection direction and sample the nearest hit

Unlike a ray tracing pipeline (which replaces rasterization entirely), ray queries work **alongside** rasterization. The scene is rendered normally, and ray queries are performed per-fragment as needed.

```
Rasterization (normal rendering)     Ray Query (shadow test)
┌──────────────────────┐            ┌──────────────────────┐
│ Draw ground, cube,   │            │ For each fragment:   │
│ sphere with forward  │            │   Cast ray toward    │
│ Lambert lighting     │            │   light source       │
│                      │            │   Hit? -> shadow     │
│ Fragment shader:     │<───────────│   Miss? -> lit       │
│   Compute N.L        │  query     │                      │
│   Apply shadow       │  TLAS      └──────────────────────┘
└──────────────────────┘
```

The acceleration structure (TLAS) is built from the same geometry used for rasterization, so no additional mesh data is needed.

**Reference:** [GPU API -- RayTracingMesh](../../api/gpu/RayTracingMesh.md), [GPU API -- RayTracingScene](../../api/gpu/RayTracingScene.md)

## Step-by-step code explanation

### 1. Data structures for UBOs

```cpp
struct CameraUBO {
    glm::mat4 projectionView;
};

struct ObjectUBO {
    glm::mat4 model;
    glm::vec4 albedo;
};

struct LightUBO {
    glm::vec4 lightPosition;   // xyz = world position
    glm::vec4 lightColor;      // rgb = color, a = intensity
    glm::vec4 ambient;         // rgb = ambient term
};
```

Three UBO structs define the per-frame data:

- **`CameraUBO`**: combined projection-view matrix (set 0, binding 0)
- **`ObjectUBO`**: per-object model matrix and albedo color (set 1, binding 0)
- **`LightUBO`**: light position, color with intensity, and ambient term (set 2, binding 0)

The `LightUBO` uses `vec4` fields for std140 alignment. The light's `.w` component stores intensity, and ambient stores the ambient color in `.rgb`.

### 2. SceneObject with RayTracingMesh

```cpp
struct SceneObject {
    std::shared_ptr<bg2e::gpu::MeshPN> mesh;
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 albedo = glm::vec4(1.0f);

    std::shared_ptr<bg2e::gpu::Buffer>      objectUbo;
    std::shared_ptr<bg2e::gpu::ResourceSet> objectSet;

    std::vector<std::shared_ptr<bg2e::gpu::RayTracingMesh>> rtMeshes;
};
```

Each scene object carries:

- **`mesh`**: the rasterization mesh (`MeshPN` = position + normal)
- **`model`** / **`albedo`**: transform and color for the object
- **`objectUbo`** / **`objectSet`**: GPU buffer and resource set for per-object data
- **`rtMeshes`**: one `RayTracingMesh` per submesh -- these are the bottom-level acceleration structures

The `RayTracingMesh` objects share the same vertex and index buffers as the rasterization mesh. No geometry is duplicated.

**Reference:** [GPU API -- RayTracingMesh: Description](../../api/gpu/RayTracingMesh.md#raytracingmeshdescription)

### 3. Ray tracing capability detection

```cpp
if (!physicalDevice->properties()->rayTracingSupported())
{
    bg2e_log_error << "This example requires hardware accelerated ray queries, "
              << "which are not supported by the selected device:\n  "
              << physicalDevice->properties()->name << bg2e_log_end;

    if (bg2e::base::PlatformTools::currentPlatform() == base::Platform::macOS &&
        backendType == gpu::BackendType::Vulkan)
    {
        bg2e_log_error << "Note that Vulkan in macOS is not compatible with ray tracing. "
              << "Please, try using Metal instead of Vulkan" << bg2e_log_end;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
}
```

Before creating any acceleration structures, the example checks that the GPU supports ray tracing. The `rayTracingSupported()` method returns `true` only when all five capability flags are set: `available`, `rayTracingPipeline`, `rayQuery`, `accelerationStructure`, and `bufferDeviceAddress`.

On macOS, Vulkan through MoltenVK does not support ray tracing. The example detects this and suggests Metal as an alternative.

**Reference:** [GPU API -- PhysicalDeviceProperties: RayTracingCapabilities](../../api/gpu/PhysicalDeviceProperties.md#raytracingcapabilities)

### 4. Pipeline layout with acceleration structure binding

```cpp
gpu::PipelineLayoutDescription layoutDesc{};
layoutDesc.resourceBindings.push_back({
    0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
layoutDesc.resourceBindings.push_back({
    1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
});
layoutDesc.resourceBindings.push_back({
    2, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1
});
layoutDesc.resourceBindings.push_back({
    2, {.vulkan = 1, .metal = 2}, gpu::ResourceType::AccelerationStructure, gpu::ShaderStage::Fragment, 1
});
layoutDesc.debugName = "Ray query shadows pipeline layout";
auto graphicsLayout = device->createPipelineLayout(layoutDesc);
```

The pipeline layout defines three descriptor sets:

| Set | Vulkan Binding | Metal Index | Type | Stage | Purpose |
|-----|---------------|-------------|------|-------|---------|
| 0 | 0 | buffer(2) | UniformBuffer | Vertex | Camera UBO |
| 1 | 0 | buffer(3) | UniformBuffer | Vertex | Per-object UBO |
| 2 | 0 | buffer(1) | UniformBuffer | Fragment | Light UBO |
| 2 | 1 | buffer(2) | AccelerationStructure | Fragment | Top-level AS |

The critical binding is **set 2, binding 1** -- the `ResourceType::AccelerationStructure`. This tells the backend to create a `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` descriptor (Vulkan) or bind an `instance_acceleration_structure` argument (Metal).

The `ShaderBinding` struct uses `.vulkan` for the descriptor binding index and `.metal` for the Metal buffer index. Metal requires buffer indices >= 1 in the fragment stage (index 0 is reserved for push constants).

**Reference:** [GPU API -- PipelineLayout: Resource bindings](../../api/gpu/PipelineLayout.md#resource-bindings), [GPU API -- ResourceSet: Metal binding rules](../../api/gpu/ResourceSet.md#metal-binding-rules)

### 5. Scene object creation with RayTracingMesh

```cpp
auto addObject = [&](bg2e::geo::MeshPN* data, const glm::mat4& model, const glm::vec4& albedo)
{
    SceneObject obj;
    obj.mesh = std::make_shared<gpu::MeshPN>();
    obj.mesh->setMeshData(*data);
    obj.mesh->build(device.get());
    obj.model = model;
    obj.albedo = albedo;

    ObjectUBO objectData{};
    objectData.model = model;
    objectData.albedo = albedo;
    obj.objectUbo = device->createBuffer("Object UBO");
    obj.objectUbo->createUniformBuffer(objectData);

    obj.objectSet = device->createResourceSet(graphicsLayout.get(), 1, "Object resource set");
    obj.objectSet->setUniformBuffer({.vulkan = 0, .metal = 3}, obj.objectUbo);
    obj.objectSet->update();

    // One RayTracingMesh per submesh, reusing the mesh GPU buffers.
    for (uint32_t s = 0; s < obj.mesh->submeshCount(); ++s)
    {
        obj.rtMeshes.push_back(
            device->createRayTracingMesh(obj.mesh->rayTracingMeshDescription(s))
        );
    }

    objects.push_back(std::move(obj));
    delete data;
};
```

The `addObject` lambda does three things:

1. **Creates the rasterization mesh**: uploads vertex/index data to the GPU via `MeshPN::build()`
2. **Creates per-object UBO and resource set**: the model matrix and albedo are uploaded once
3. **Creates RayTracingMesh objects**: one per submesh, using `rayTracingMeshDescription(s)` which returns a description referencing the existing GPU buffers

The `rayTracingMeshDescription()` method from `MeshGeneric<T>` automatically resolves the vertex stride, position offset, and submesh index range. The `RayTracingMesh` does not own or duplicate the vertex/index buffers -- the caller must keep the mesh alive.

Three objects are added:

```cpp
addObject(bg2e::geo::createPlanePN(12.0f, 12.0f),
          glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
          glm::vec4(0.75f, 0.75f, 0.78f, 1.0f));

addObject(bg2e::geo::createCubePN(1.2f, 1.2f, 1.2f),
          glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.6f, 0.4f)),
          glm::vec4(0.85f, 0.25f, 0.20f, 1.0f));

addObject(bg2e::geo::createSpherePN(0.75f, 32, 24),
          glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.75f, -0.3f)),
          glm::vec4(0.20f, 0.45f, 0.85f, 1.0f));
```

**Reference:** [GPU API -- Mesh: rayTracingMeshDescription()](../../api/gpu/Mesh.md)

### 6. Building bottom-level acceleration structures (BLAS)

```cpp
device->immediateSubmit([&](gpu::CommandBuffer* cmd)
{
    for (auto& obj : objects)
    {
        for (auto& rtMesh : obj.rtMeshes)
        {
            cmd->buildRayTracingMesh(rtMesh.get());
        }
    }
});
```

The BLAS build is a GPU command. `immediateSubmit()` records the commands and submits them synchronously -- the function returns only after the GPU finishes building all acceleration structures.

Each `buildRayTracingMesh()` call records the backend-specific build command:
- **Vulkan**: `vkCmdBuildAccelerationStructuresKHR` with the BLAS handle, geometry data, and scratch buffer
- **Metal**: an acceleration structure build on a `MTL::AccelerationStructureCommandEncoder`

The acceleration structure storage and scratch buffers are allocated at `createRayTracingMesh()` time; only the actual build is a GPU command.

**Reference:** [GPU API -- RayTracingMesh: Usage](../../api/gpu/RayTracingMesh.md#usage), [GPU API -- Device::immediateSubmit()](../../api/gpu/Device.md)

### 7. Top-level acceleration structure (TLAS) with RayTracingScene

```cpp
auto rayTracingScene = device->createRayTracingScene("Shadow ray tracing scene");
cleanup.push(rayTracingScene);

uint32_t instanceId = 0;
auto populateInstances = [&]()
{
    rayTracingScene->clearInstances();
    instanceId = 0;
    for (auto& obj : objects)
    {
        for (auto& rtMesh : obj.rtMeshes)
        {
            rayTracingScene->addInstance(rtMesh.get(), obj.model, instanceId++, 0xFF);
        }
    }
};
```

The `RayTracingScene` manages the top-level acceleration structure. The CPU-side instance list is built using:

- **`clearInstances()`**: resets the instance list (called each frame)
- **`addInstance()`**: adds one instance of a `RayTracingMesh` with its world transform, instance ID, and visibility mask

Each submesh gets its own instance with its object's model matrix. The `instanceId` is a sequential counter; the `0xFF` mask means the instance is visible to all rays (all bits set).

```cpp
// Initial build so the acceleration structure handle exists before binding.
populateInstances();
device->immediateSubmit([&](gpu::CommandBuffer* cmd)
{
    rayTracingScene->buildOrUpdate(cmd);
});
```

The initial build is required so the TLAS handle exists before it is bound into the light resource set. `buildOrUpdate()` records the GPU build command using the current CPU-side instance list.

**Reference:** [GPU API -- RayTracingScene: Usage](../../api/gpu/RayTracingScene.md#usage)

### 8. Resource set with acceleration structure binding

```cpp
auto lightUbo = device->createBuffer("Light UBO");
lightUbo->createUniformBuffer(LightUBO{});
cleanup.push(lightUbo);

auto lightSet = device->createResourceSet(graphicsLayout.get(), 2, "Light resource set");
lightSet->setUniformBuffer({.vulkan = 0, .metal = 1}, lightUbo);
lightSet->setRayTracingScene({.vulkan = 1, .metal = 2}, rayTracingScene.get());
lightSet->update();
cleanup.push(lightSet);
```

The light resource set binds two resources to descriptor set 2:

1. **Light UBO** at binding 0: the light position, color, and ambient values
2. **RayTracingScene** at binding 1: the top-level acceleration structure

The `setRayTracingScene()` method writes the acceleration structure descriptor:
- **Vulkan**: chains a `VkWriteDescriptorSetAccelerationStructureKHR` into the `VkWriteDescriptorSet`
- **Metal**: binds the instance acceleration structure at the specified buffer index and marks primitive acceleration structures as resident via `useResource`

After `update()`, the descriptor set is ready for binding. The UBO contents can be updated each frame via `updateUniformBuffer()` without re-calling `update()`.

**Reference:** [GPU API -- ResourceSet::setRayTracingScene()](../../api/gpu/ResourceSet.md#virtualvoidsetraytracingscene)

### 9. Graphics pipeline

```cpp
gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
pipelineDesc.colorFormat    = surface->colorFormat();
pipelineDesc.depthFormat    = surface->depthFormat();
pipelineDesc.cullMode       = gpu::CullMode::None;
pipelineDesc.debugName      = "Ray query shadows pipeline";
pipelineDesc.addVertexBufferDescription(gpu::MeshPN::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);
```

The pipeline is a standard forward rasterization pipeline:

- **`cullMode = CullMode::None`**: disables face culling so single-sided primitives (plane) are fully lit
- **`depthFormat`**: uses the surface depth format for proper 3D occlusion
- **Vertex format**: `MeshPN` (position + normal) -- no texture coordinates needed for this example

The pipeline does not know about ray tracing -- it simply renders the scene with Lambert lighting. The ray query happens in the fragment shader.

**Reference:** [GPU API -- GraphicsPipeline](../../api/gpu/GraphicsPipeline.md)

### 10. Render loop with dynamic TLAS rebuild

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

    // Orbit the point light above the objects so the shadows move.
    const float orbitRadius = 3.2f;
    LightUBO lightData{};
    lightData.lightPosition = glm::vec4(
        orbitRadius * std::cos(t),
        4.0f,
        orbitRadius * std::sin(t),
        1.0f
    );
    lightData.lightColor = glm::vec4(1.0f, 0.97f, 0.9f, 1.6f);
    lightData.ambient    = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f);
    lightUbo->updateUniformBuffer(lightData);

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

    cmd->begin();

    // Rebuild the top-level acceleration structure before rendering.
    populateInstances();
    rayTracingScene->buildOrUpdate(cmd.get());

    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, gpu::Color(0.04f, 0.05f, 0.07f, 1.0f));
    cmd->clearDepth(1.0f);

    cmd->bindPipeline(pipeline.get());
    cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
    cmd->bindResourceSet(pipeline.get(), 2, lightSet.get());

    for (auto& obj : objects)
    {
        cmd->bindResourceSet(pipeline.get(), 1, obj.objectSet.get());
        obj.mesh->draw(cmd.get());
    }

    cmd->endRendering();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());
}
```

The render loop has two key elements for ray tracing:

#### Dynamic TLAS rebuild

```cpp
populateInstances();
rayTracingScene->buildOrUpdate(cmd.get());
```

The TLAS is rebuilt **every frame** before `beginRendering()`. This is required because:

1. `buildOrUpdate()` must be called **outside** an active rendering scope (before `beginRendering()`)
2. The API requires rebuilding when the instance list changes (even if geometry is static, the API does not track changes)

In this example the geometry is static (only the light moves), so the instance list is identical each frame. The rebuild is done to exercise the dynamic build path. In production, you could skip the rebuild if the instance list has not changed.

#### Per-object resource set binding

```cpp
for (auto& obj : objects)
{
    cmd->bindResourceSet(pipeline.get(), 1, obj.objectSet.get());
    obj.mesh->draw(cmd.get());
}
```

Each object has its own resource set for the model matrix and albedo. The camera (set 0) and light (set 2) resource sets are bound once before the loop, since they are shared across all objects.

**Reference:** [GPU API -- RayTracingScene::buildOrUpdate()](../../api/gpu/RayTracingScene.md#virtualvoidbuildorupdate)

## Shader code explanation

### Vertex Shader (`shadow.vert.glsl`)

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vAlbedo;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projectionView;
} camera;

layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec4 albedo;
} object;

void main()
{
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    vWorldPos     = worldPos.xyz;
    vWorldNormal  = mat3(object.model) * inNormal;
    vAlbedo       = object.albedo.rgb;

    gl_Position = camera.projectionView * worldPos;
}
```

The vertex shader is straightforward:

1. **Transforms** the vertex position to world space using the per-object model matrix
2. **Transforms** the normal to world space using the upper-left 3x3 of the model matrix (valid because the example uses uniform scaling)
3. **Passes** world position, normal, and albedo color to the fragment shader as interpolated varyings
4. **Projects** the world position to clip space using the combined projection-view matrix

No ray tracing data is needed in the vertex shader -- the ray query is performed entirely in the fragment shader.

### Fragment Shader (`shadow.frag.glsl`)

```glsl
#version 460
#extension GL_EXT_ray_query : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vAlbedo;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform LightUBO {
    vec4 lightPosition;   // xyz = world position
    vec4 lightColor;      // rgb = color, a = intensity
    vec4 ambient;         // rgb = ambient term
} light;

layout(set = 2, binding = 1) uniform accelerationStructureEXT topLevelAS;

void main()
{
    vec3 N = normalize(vWorldNormal);

    vec3  toLight   = light.lightPosition.xyz - vWorldPos;
    float distance  = length(toLight);
    vec3  lightDir  = toLight / distance;
    float ndotl     = max(dot(N, lightDir), 0.0);

    // Cast a visibility ray from the shaded point toward the light. If anything
    // is hit before the light, the point is in shadow.
    const float bias = 0.02;
    float shadow = 1.0;

    rayQueryEXT rq;
    rayQueryInitializeEXT(
        rq,
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        vWorldPos + N * bias,
        bias,
        lightDir,
        distance - bias
    );
    rayQueryProceedEXT(rq);
    if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        shadow = 0.15;
    }

    vec3 diffuse = vAlbedo * light.lightColor.rgb * light.lightColor.a * ndotl * shadow;
    vec3 ambient = vAlbedo * light.ambient.rgb;

    outColor = vec4(ambient + diffuse, 1.0);
}
```

This is the core of the example. The fragment shader performs two tasks: standard Lambert lighting and a ray query for shadow testing.

#### Ray query initialization

```glsl
rayQueryEXT rq;
rayQueryInitializeEXT(
    rq,
    topLevelAS,
    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
    0xFF,
    vWorldPos + N * bias,
    bias,
    lightDir,
    distance - bias
);
```

The `rayQueryEXT` variable holds the query state. `rayQueryInitializeEXT()` configures it:

| Parameter | Value | Meaning |
|-----------|-------|---------|
| `rq` | -- | The query object |
| `topLevelAS` | bound TLAS | The acceleration structure to query |
| `gl_RayFlagsTerminateOnFirstHitEXT \| gl_RayFlagsOpaqueEXT` | flags | Stop at the first hit (no any-hit shader), treat all geometry as opaque |
| `0xFF` | mask | Instance mask (matches the `0xFF` used in `addInstance`) |
| `vWorldPos + N * bias` | origin | Ray origin offset along the normal to avoid self-intersection |
| `bias` | tMin | Minimum ray distance (small value to skip the originating surface) |
| `lightDir` | direction | Unit vector from fragment toward the light |
| `distance - bias` | tMax | Maximum ray distance (distance to light minus bias) |

The **bias** is critical for preventing shadow acne -- a common artifact where a fragment shadows itself due to floating-point imprecision. By offsetting the ray origin along the normal and clamping tMin/tMax, the ray starts slightly above the surface.

#### Ray query processing

```glsl
rayQueryProceedEXT(rq);
if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT)
{
    shadow = 0.15;
}
```

`rayQueryProceedEXT()` advances the query through the acceleration structure. For simple shadow queries with `TerminateOnFirstHit`, a single call is sufficient -- the query stops as soon as any geometry is intersected.

`rayQueryGetIntersectionTypeEXT(rq, true)` checks the **committed** intersection (the final result after any shaders run). If the type is not `None`, something was hit between the fragment and the light, so the fragment is in shadow.

When in shadow, the diffuse term is multiplied by 0.15 (not 0) to produce a soft shadow appearance rather than complete darkness. The ambient term is always applied regardless of shadow state.

#### Lambert lighting with shadows

```glsl
vec3 diffuse = vAlbedo * light.lightColor.rgb * light.lightColor.a * ndotl * shadow;
vec3 ambient = vAlbedo * light.ambient.rgb;

outColor = vec4(ambient + diffuse, 1.0);
```

The final color combines:

- **Diffuse**: albedo multiplied by light color and intensity, modulated by `N.L` and the shadow factor
- **Ambient**: albedo multiplied by ambient light (always visible, independent of shadows)

The shadow factor is either 1.0 (fully lit) or 0.15 (in shadow), producing hard shadow edges.

## Key concepts

### Acceleration structure hierarchy

The ray tracing acceleration structure is a two-level hierarchy:

```
Top-Level Acceleration Structure (TLAS)
  ├── Instance 0: Ground plane (BLAS)
  ├── Instance 1: Cube (BLAS)
  └── Instance 2: Sphere (BLAS)
```

Each **BLAS** (`RayTracingMesh`) contains the geometry for a single submesh -- vertex buffer, index buffer, and the build configuration. Each **instance** in the TLAS (`RayTracingScene`) references a BLAS with a world transform and metadata (instance ID, mask).

The TLAS is what gets bound to the shader. When a ray is cast, the GPU traverses the TLAS to find which instance is hit, then resolves the specific triangle within that instance's BLAS.

**Reference:** [GPU API -- RayTracingMesh](../../api/gpu/RayTracingMesh.md), [GPU API -- RayTracingScene](../../api/gpu/RayTracingScene.md)

### BLAS build vs. TLAS build

| Operation | When | How | What it does |
|-----------|------|-----|--------------|
| BLAS build | Once per mesh | `device->immediateSubmit()` with `cmd->buildRayTracingMesh()` | Builds the acceleration structure for one submesh's geometry |
| TLAS build | Every frame | `cmd->begin(); rayTracingScene->buildOrUpdate(cmd);` before `beginRendering()` | Builds the instance-level structure from the current instance list |

BLAS builds are expensive and only needed when geometry changes. TLAS builds are cheaper and may be needed every frame if instances move or change.

### Shadow bias

The shadow bias (`0.02`) prevents self-intersection artifacts:

```glsl
vWorldPos + N * bias   // offset origin along normal
bias                    // tMin
distance - bias         // tMax
```

Without the bias, a fragment might intersect the surface it sits on, producing false shadow results. The offset pushes the ray origin slightly above the surface, and the tMin/tMax range ensures the ray only tests geometry between the surface and the light.

The bias value should be tuned based on scene scale -- too small causes acne, too large causes peter-panning (shadows detaching from their casters).

### Ray flags

```glsl
gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT
```

- **`TerminateOnFirstHitEXT`**: stops traversal at the first intersection. For shadows, we only need to know if *anything* is in the way -- the closest hit is irrelevant.
- **`OpaqueEXT`**: treats all geometry as opaque, skipping any-hit shaders. This is an optimization since we do not have transparent geometry.

These flags make the ray query as fast as possible for the shadow use case.

### Dynamic TLAS rebuild pattern

```cpp
cmd->begin();

// 1. Rebuild TLAS (outside rendering scope)
populateInstances();
rayTracingScene->buildOrUpdate(cmd.get());

// 2. Begin rendering (inside rendering scope)
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->beginRendering(frame.get());
// ... draw ...
cmd->endRendering();

cmd->end();
```

The TLAS build **must** happen outside the rendering scope (between `cmd->begin()` and `cmd->beginRendering()`). This is because `buildOrUpdate()` may need to perform barrier transitions that are incompatible with an active rendering pass.

**Reference:** [GPU API -- RayTracingScene::buildOrUpdate(): Build/update the top-level acceleration structure](../../api/gpu/RayTracingScene.md#virtualvoidbuildorupdate)

### Resource set update without re-calling update()

The light UBO is updated each frame:

```cpp
lightUbo->updateUniformBuffer(lightData);
```

This only updates the buffer contents -- the resource set does not need to be re-created or re-updated. The descriptor still points to the same buffer, and the new data is visible to the shader on the next draw call. This is the standard pattern for per-frame UBO updates.

## Next steps

- **[GPU API -- RayTracingMesh](../../api/gpu/RayTracingMesh.md)** -- Full reference for bottom-level acceleration structures
- **[GPU API -- RayTracingScene](../../api/gpu/RayTracingScene.md)** -- Full reference for top-level acceleration structures and instance management
- **[GPU API -- ResourceSet](../../api/gpu/ResourceSet.md)** -- Full reference for binding acceleration structures to shaders
- **[GPU API -- PhysicalDeviceProperties](../../api/gpu/PhysicalDeviceProperties.md)** -- Full reference for ray tracing capability detection
