# Tutorial 07: Ray Query Shadows

This tutorial adapts [GPU Ray Query Shadows](../gpu/11_ray_query_shadows.md)
to `bg2e::draw`. A ground plane, cube, and sphere are rasterized with Lambert
lighting. Each fragment queries an acceleration structure toward an orbiting
point light to determine whether its diffuse lighting is shadowed.

**Source:** [examples/draw/07_ray_query_shadows/src/main.cpp](../../../examples/draw/07_ray_query_shadows/src/main.cpp)

## What you will learn

- How to check the selected device's ray tracing support before loading shaders
- How raster meshes supply geometry for bottom-level acceleration structures
- How to populate and build a top-level ray tracing scene
- How to bind an acceleration structure alongside a light uniform buffer
- How to rebuild a TLAS outside the active rendering scope
- Why mutable acceleration structures need frame-slot isolation
- How fragment ray queries produce hard shadows on Vulkan and Metal
- How to retain pause, background, and UI controls in a draw application

## Prerequisites

- Completed [03_uniform_buffers](03_uniform_buffers.md)
- Familiarity with the scene delegate lifecycle from [01_window_ui](01_window_ui.md)
- A configured bg2e-native build environment with a valid Vulkan SDK
- A GPU/backend satisfying `PhysicalDeviceProperties::rayTracingSupported()`
- Compiled and packaged GLSL shaders and, on macOS, Metal shaders
- The Metal compiler toolchain on macOS

The capability check preserves the source example's policy. It uses the engine's
full ray tracing capability predicate, which includes ray query, acceleration
structure, buffer device address, and ray tracing pipeline support, even though
this example does not create a ray tracing pipeline. Unsupported devices cause
a descriptive startup error before scene shaders or acceleration structures
are created. On macOS, the Vulkan error suggests trying Metal on a compatible
Mac. There is no fallback rendering mode.

The example uses only procedural geometry and requires no texture assets.
Rendering uses production draw over the selected GPU backend.

## Understanding the example

The scene preserves the source GPU example's geometry, camera, light, and
shader behavior:

| Object | Geometry | Position | Albedo |
|--------|----------|----------|--------|
| Ground | 12x12 plane | Origin | Light gray |
| Cube | 1.2x1.2x1.2 cube | (-1.3, 0.6, 0.4) | Red |
| Sphere | Radius 0.75, 32x24 tessellation | (1.3, 0.75, -0.3) | Blue |

The camera looks from (4.5, 4.0, 5.5) toward (0.0, 0.6, 0.0), with a 55-degree
perspective. The light orbits with radius 3.2 at height 4.0. Only the light moves;
the meshes, object transforms, and albedo values remain static.

Rasterization produces visible fragments. Ray queries only test visibility
between those fragments and the light. No ray generation shader, shader binding
table, or dedicated ray tracing pipeline is needed. The shadow factor is either
1.0 or 0.15, producing hard boundaries with a reduced diffuse contribution in
shadow; ambient illumination is always added.

The source rebuilds its TLAS every frame to demonstrate dynamic scene building.
This adaptation preserves that demonstration on every active scene frame and
uses a separate TLAS, light UBO, and matching resource set for each frame slot.
The bottom-level geometry is shared because it is built once and stays immutable.

## Step-by-step code explanation

### 1. Reuse Application/MainLoop and validate capabilities

The entry point follows previous draw examples: install delegates through
`app::Application`, then run `app::MainLoop` with `draw::EngineConfig`. The
optional backend arguments are `--backend=vulkan` and `--backend=metal`; the
default is Metal on macOS and Vulkan elsewhere.

`initScene()` accesses the device already selected by draw:

```cpp
const auto properties = _engine->physicalDevice()->properties();
if (!properties->rayTracingSupported())
{
    std::string message = "This example requires hardware accelerated ray queries. Device: " + properties->name;
#ifdef BG2E_IS_MAC
    if (_engine->backendType() == gpu::BackendType::Vulkan)
    {
        message += ". Try --backend=metal on a compatible Mac.";
    }
#endif
    throw std::runtime_error(message);
}
```

The top-level exception handler prints the failure and returns a nonzero status.
The controller stores the device name for the information window.

### 2. Load the shader pair and define bindings

`gpu::ShaderLib` loads `shadow` from `shaders/draw_ray_query_shadows`.
The layout preserves the source's descriptor and Metal buffer indices:

| Resource | Stage | Vulkan set/binding | Metal buffer |
|----------|-------|--------------------|--------------|
| Camera UBO | Vertex | 0 / 0 | 2 |
| Object UBO | Vertex | 1 / 0 | 3 |
| Light UBO | Fragment | 2 / 0 | 1 |
| RayTracingScene | Fragment | 2 / 1 | 2 |

The acceleration structure binding uses `gpu::ResourceType::AccelerationStructure`.
Metal indices are stage-specific, so vertex camera buffer 2 and fragment
acceleration structure buffer 2 do not conflict.

The camera contains a projection-view matrix, each object contains a model
matrix and albedo vec4, and the light contains three vec4 values for position,
color/intensity, and ambient color. Camera and object resources remain persistent;
light resources use a ring because their contents change each active frame.

### 3. Build raster meshes and bottom-level geometry

Each object owns a `gpu::MeshPN`, which contains positions and normals.
The application takes ownership of the procedural CPU mesh with `unique_ptr`,
copies its data into the GPU mesh wrapper, and builds its vertex/index buffers.
Static object uniforms and their set 1 resource sets are created once.

A `gpu::RayTracingMesh` is created for each submesh:

```cpp
for (uint32_t submesh = 0; submesh < obj.mesh.submeshCount(); ++submesh)
{
    auto rtMesh = device->createRayTracingMesh(obj.mesh.rayTracingMeshDescription(submesh));
    _cleanup->push(rtMesh);
    obj.rtMeshes.push_back(std::move(rtMesh));
}
```

`rayTracingMeshDescription()` describes the existing GPU buffers, position
layout, and submesh index range. These buffers are borrowed, so the raster mesh
must outlive the referencing ray tracing mesh.

One synchronous `device->immediateSubmit()` records `buildRayTracingMesh()` for
all objects/submeshes. When it returns, their bottom-level acceleration
structures (BLAS) are ready for use by top-level instances.

### 4. Populate a top-level instance list

The same helper populates every slot's `gpu::RayTracingScene`:

```cpp
void populateInstances(gpu::RayTracingScene* scene)
{
    scene->clearInstances();
    uint32_t instanceId = 0;
    for (auto& obj : _objects)
    {
        for (auto& rtMesh : obj->rtMeshes)
        {
            scene->addInstance(rtMesh.get(), obj->model, instanceId++, 0xFF);
        }
    }
}
```

Every instance references a built BLAS with its object's world transform.
Sequential instance IDs distinguish submeshes; mask `0xFF` matches the shader's
ray visibility mask. The ray tracing scene forms the top-level acceleration
structure (TLAS) queried by the fragment shader.

### 5. Initialize frame-slot resources during resize

Draw calls `resize()` before the first scene render and after draining GPU work
when the retained target changes. The delegate updates the camera's aspect
ratio and creates its own depth image to accompany draw's color-only target.

It then creates `_sceneRing`, with one populated RayTracingScene per in-flight
slot. An immediate submission builds all TLAS objects before any resource set
binds them, ensuring native acceleration structure handles already exist.

Light buffers and resource sets are created next:

```cpp
_lightSetRing.create(surface, [this, device](uint32_t slot)
{
    auto set = device->createResourceSet(_layout.get(), 2,
                                         "Light resource set slot[" + std::to_string(slot) + "]");
    set->setUniformBuffer({.vulkan = 0, .metal = 1}, _lightUboRing.sharedAt(slot));
    set->setRayTracingScene({.vulkan = 1, .metal = 2}, _sceneRing.at(slot));
    set->update();
    return set;
});
```

Each set points to its slot's light buffer and TLAS. The instance count and
geometry remain fixed, so later rebuilds reuse native handles and require no
descriptor updates. A future example that increases capacity must also account
for handle replacement and rebinding after the appropriate GPU wait.

The graphics pipeline uses `MeshPN`, the surface color format, the owned depth
format, and `CullMode::None`, preserving the single-sided plane behavior.

### 6. Update the orbiting light in the current slot

`update()` accumulates `context.deltaSeconds` into scene animation time and
writes the current slot's UBO:

```cpp
_time += context.deltaSeconds;
LightUBO light{};
const float orbitRadius = 3.2f;
light.lightPosition = glm::vec4(orbitRadius * std::cos(_time), 4.0f,
                                 orbitRadius * std::sin(_time), 1.0f);
light.lightColor = glm::vec4(1.0f, 0.97f, 0.9f, 1.6f);
light.ambient = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f);
_lightUboRing.at(context.frameSlot)->updateUniformBuffer(light);
```

Draw has already waited for that slot before calling update/render. Updating
its light buffer cannot overwrite another in-flight frame's uniform data.
Using scene delta time also freezes the light's orbit while paused.

### 7. Rebuild the current TLAS before rendering

At the start of `render()`, the delegate selects the same frame slot:

```cpp
auto& command = context.commandBuffer;
auto* scene = _sceneRing.at(context.frameSlot);
populateInstances(scene);
scene->buildOrUpdate(&command);
```

The build is outside `beginRendering()` and precedes fragment reads. GPU backend
commands handle the build and its visibility to later ray queries. The ring
also isolates mutable instance input, scratch storage, and TLAS output from
other frames still executing on the GPU.

Rebuilding is retained to exercise the API. Static instances with only a moving
light do not inherently require a new TLAS build every frame.

### 8. Rasterize into the retained target

The delegate transitions its depth image, begins rendering with
`context.colorTarget` and depth, and clears both attachments. It binds the
camera and matching light/TLAS set, then draws each object's mesh:

```cpp
command.bindPipeline(_pipeline.get());
command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
command.bindResourceSet(_pipeline.get(), 2, _lightSetRing.at(context.frameSlot));
for (auto& obj : _objects)
{
    command.bindResourceSet(_pipeline.get(), 1, obj->objectSet.get());
    obj->mesh.draw(&command);
}
command.endRendering();
```

Draw manages command submission, retained scene copying, UI composition, and
presentation. The delegate does not acquire or present a surface frame.

### 9. Keep the information window and pause behavior

The UI displays backend, device, ray query status, and scene/UI counters, with
Background, Pause scene, and Refresh scene controls. It requests scene frames
while active so the light and shadows animate.

Pause preserves the retained image and stops light updates, TLAS rebuilds, and
scene draws while UI presentation continues. Background/refresh changes remain
pending until resume. Paused resize follows the retained target clear-color
behavior from tutorial 01.

### 10. Release resources in dependency order

Resize cleans the graphics pipeline, depth image, and three slot rings after
draw drains GPU work. Ring cleanup releases light resource sets before the
referenced light buffers and ray tracing scenes. New rings use the current
surface frame count. Persistent BLAS and raster meshes remain available.

Shutdown follows the same ring cleanup, then flushes the delegate's local
CleanupManager for BLAS, object/camera resources, layouts, and shaders. Finally,
it cleans raster meshes and their vertex/index buffers. This ensures TLAS is
released before BLAS, and BLAS before its borrowed geometry buffers.

## Shader code explanation

### Vertex transformation

The vertex shader multiplies positions by the object's model matrix, transforms
normals using its upper 3x3, and passes world position, normal, and albedo to the
fragment shader. The example uses translations, so this normal transformation
is sufficient. The camera projection-view matrix produces clip-space positions.

### Vulkan fragment ray query

The GLSL fragment shader requires `GL_EXT_ray_query`. It normalizes the surface
normal and light direction, then initializes a visibility query:

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
    distance - bias);
rayQueryProceedEXT(rq);
if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT)
{
    shadow = 0.15;
}
```

The source shader uses opaque triangle geometry and terminates at the first
hit. The `0.02` bias offsets the origin and excludes the originating surface;
the maximum distance limits occlusion to geometry before the light.

### Metal fragment intersector

The Metal shader includes `metal_raytracing` and binds an
`instance_acceleration_structure`. Its `intersector<instancing, triangle_data>`
forces opaque triangle geometry and accepts any intersection. It queries the
same biased ray segment and applies `shadow = 0.15` when the result is a hit.

Both backends compute the same final color:

```glsl
vec3 diffuse = vAlbedo * light.lightColor.rgb * light.lightColor.a * ndotl * shadow;
vec3 ambient = vAlbedo * light.ambient.rgb;
outColor = vec4(ambient + diffuse, 1.0);
```

The example demonstrates hard shadows; a reduced shadow intensity does not
produce a soft penumbra. All four shader sources preserve the GPU example's
code, with final newline normalization only.

## Key concepts

- **BLAS versus TLAS:** submesh geometry is built once; instance structures are
  rebuilt for each active scene frame.
- **Rasterization plus visibility rays:** a regular graphics pipeline shades
  fragments and queries the TLAS, without a dedicated ray tracing pipeline.
- **Frame-slot isolation:** the light UBO, TLAS, and their set share the same
  `FrameContext::frameSlot`; immutable mesh and object resources remain shared.
- **Build outside rendering:** acceleration builds precede `beginRendering()`.
- **Lifetime dependencies:** descriptors outlive no referenced resources;
  acceleration structures are released before borrowed geometry buffers.

## Complete source

See [main.cpp](../../../examples/draw/07_ray_query_shadows/src/main.cpp) and the
[shader directory](../../../examples/draw/07_ray_query_shadows/shaders/).

## Building and running

The [CMakeLists.txt](../../../examples/draw/07_ray_query_shadows/CMakeLists.txt)
preserves the GPU example's explicit GLSL compilation with
`--target-env vulkan1.2` for both graphics shader stages and the ShaderLib entry
names `vertMain`/`fragMain`. The general shader helper targets a lower environment
for ordinary graphics stages, so this example retains the source's custom
commands for fragment ray queries. Metal compilation and resource packaging
also follow the source, with target name `draw_ray_query_shadows`.

Use your configured build directory, replacing `build` as needed:

```sh
cmake --build build --target draw_ray_query_shadows --config Debug
```

On macOS:

```sh
./bin/macos/draw_ray_query_shadows.app/Contents/MacOS/draw_ray_query_shadows --backend=metal
```

On Linux, run `./bin/linux/draw_ray_query_shadows --backend=vulkan`.
On Windows, run `draw_ray_query_shadows.exe --backend=vulkan` under `bin/windows`.
The selected GPU/backend must pass the capability check. The macOS target
compiles Metal shaders as part of the build.

To inspect the result:

1. Observe the plane, red cube, blue sphere, and moving hard shadows.
2. Pause and confirm that the light/shadows freeze while UI counters advance.
3. Change Background while paused, then resume to apply the change.
4. Resize while active to check camera aspect ratio, depth, and rebuilt rings.
5. Select an unsupported backend/device to inspect the descriptive startup error.

## Next steps

- [Deferred Cleanup](08_deferred_cleanup.md) — Replace geometry while safely retiring previously submitted GPU resources.
- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Ray Query Shadows](../gpu/11_ray_query_shadows.md) — Compare the lower-level implementation.
- [RayTracingMesh](../../api/gpu/RayTracingMesh.md) — Bottom-level acceleration geometry.
- [RayTracingScene](../../api/gpu/RayTracingScene.md) — Instance lists and top-level builds.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Persistent resources per frame slot.
