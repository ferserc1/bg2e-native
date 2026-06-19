# RayTracingScene

**Header:** `<bg2e/gpu/RayTracingScene.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API RayTracingScene : public DeviceResource {
public:
    explicit RayTracingScene(Device* device);
    ~RayTracingScene() override = default;

    // CPU-side instance list (common to all backends)
    void clearInstances();
    void addInstance(gpu::RayTracingMesh* mesh, const glm::mat4& transform,
                     uint32_t instanceId, uint32_t mask = 0xFF);

    const std::vector<RayTracingInstance>& instances() const;
    size_t instanceCount() const;

    // Build/update the top-level acceleration structure from the instance list
    virtual void buildOrUpdate(gpu::CommandBuffer* cmd) = 0;

    // From DeviceResource
    virtual bool isValid() const = 0;
    virtual void cleanup()       = 0;
};
```

`RayTracingScene` is the **top-level acceleration structure** (TLAS): a set of
instances of [`RayTracingMesh`](RayTracingMesh.md) objects, each with its own
world transform.

The CPU-side instance list lives in this object — **not** inside drawables —
because the scene instance data is dynamic. `RayTracingScene` owns the reusable
internal GPU data needed to build/update the scene:

- the current CPU-side instance list;
- a reusable GPU instance buffer;
- reusable scratch / build buffers;
- the backend acceleration structure object.

Internal buffers follow a **capacity-based reuse** model: they are reused while
the instance count fits the current capacity, and only recreated (grown) when
the instance count exceeds it.

Created via [`Device::createRayTracingScene()`](Device.md).

---

## RayTracingInstance

One instance of a `RayTracingMesh` inside the scene.

```cpp
struct RayTracingInstance {
    gpu::RayTracingMesh* mesh       = nullptr;          // bottom-level structure to instance
    glm::mat4            transform  = glm::mat4(1.0f);  // world transform
    uint32_t            instanceId  = 0;                // user instance id
    uint32_t            mask        = 0xFF;             // visibility mask
};
```

| Field        | Type                  | Description                                          |
|--------------|-----------------------|------------------------------------------------------|
| `mesh`       | `gpu::RayTracingMesh*`| The bottom-level structure being instanced.          |
| `transform`  | `glm::mat4`           | World transform applied to the instance.             |
| `instanceId` | `uint32_t`            | User instance id (Vulkan `instanceCustomIndex`).     |
| `mask`       | `uint32_t`            | Visibility mask tested against the ray's mask.       |

---

## Methods

### `void clearInstances()`

Clears the CPU-side instance list. Call this at the start of each frame (or
whenever the scene changes) before re-adding instances.

### `void addInstance(gpu::RayTracingMesh* mesh, const glm::mat4& transform, uint32_t instanceId, uint32_t mask = 0xFF)`

Appends an instance to the CPU-side list. The referenced `RayTracingMesh` must
be built (`isBuilt()`) before `buildOrUpdate()` is called.

| Parameter    | Type                  | Default | Description                          |
|--------------|-----------------------|---------|--------------------------------------|
| `mesh`       | `gpu::RayTracingMesh*`| --      | Bottom-level structure to instance.  |
| `transform`  | `const glm::mat4&`    | --      | World transform of the instance.     |
| `instanceId` | `uint32_t`            | --      | User instance id.                    |
| `mask`       | `uint32_t`            | `0xFF`  | Visibility mask.                     |

### `const std::vector<RayTracingInstance>& instances() const`

Returns the current CPU-side instance list.

### `size_t instanceCount() const`

Returns the number of instances currently in the list.

### `virtual void buildOrUpdate(gpu::CommandBuffer* cmd) = 0`

Builds or updates the top-level acceleration structure from the current instance
list, recording the GPU commands into `cmd`. Must be called **outside** an
active rendering scope (before `beginRendering()`). Internally this is
equivalent to [`cmd->buildRayTracingScene(this)`](CommandBuffer.md).

The instance/scratch/acceleration-structure buffers are reused when the instance
count fits the current capacity, and grown otherwise.

### `virtual bool isValid() const = 0`

Returns `true` once the top-level acceleration structure object exists (after
the first build).

### `virtual void cleanup() = 0`

Releases the acceleration structure and all reusable internal buffers. Must be
called before `Device::cleanup()`.

---

## Resource binding

A `RayTracingScene` is bound into a shader through the normal
[`ResourceSet`](ResourceSet.md) system, as a
`ResourceType::AccelerationStructure` binding:

```cpp
lightSet->setRayTracingScene({.vulkan = 1, .metal = 2}, rayTracingScene.get());
lightSet->update();
```

- **Vulkan:** a `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` descriptor at the
  configured set/binding (e.g. GLSL `accelerationStructureEXT`).
- **Metal:** an `instance_acceleration_structure` argument at the configured
  Metal buffer index. The referenced primitive acceleration structures are made
  resident automatically (`useResource`) at bind time.

---

## Usage

```cpp
// Populate the instance list (one instance per submesh, with its world matrix).
auto populate = [&]() {
    rayTracingScene->clearInstances();
    uint32_t id = 0;
    for (auto& obj : objects)
        for (auto& rtMesh : obj.rtMeshes)
            rayTracingScene->addInstance(rtMesh.get(), obj.model, id++, 0xFF);
};

// Initial build so the acceleration structure handle exists before binding.
populate();
device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
    rayTracingScene->buildOrUpdate(cmd);
});

// Bind into the fragment shader's resource set.
lightSet->setRayTracingScene({.vulkan = 1, .metal = 2}, rayTracingScene.get());
lightSet->update();

// Per frame: rebuild before rendering (geometry may be static; the API is not).
cmd->begin();
populate();
rayTracingScene->buildOrUpdate(cmd.get());   // outside the rendering scope
cmd->beginRendering(frame.get());
// ... draw, casting visibility rays against the bound scene ...
```

See `examples/gpu/11_ray_query_shadows` for the full example.

---

## vk::RayTracingScene

**Header:** `<bg2e/gpu/vk/RayTracingScene.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::RayTracingScene`

```cpp
class RayTracingScene : public gpu::RayTracingScene {
public:
    RayTracingScene(vk::Device* device, const std::string& debugName = {});

    void buildOrUpdate(gpu::CommandBuffer* cmd) override;
    void cleanup() override;
    bool isValid() const override;

    void build(VkCommandBuffer cmd);   // records the TLAS build

    VkAccelerationStructureKHR handle() const;
};
```

Vulkan top-level acceleration structure
(`VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR`). The instance buffer holds one
`VkAccelerationStructureInstanceKHR` per instance (transform in row-major 3x4,
`instanceCustomIndex`, `mask`, and the BLAS device address from
`vk::RayTracingMesh::deviceAddress()`). The build is recorded with
`vkCmdBuildAccelerationStructuresKHR`, followed by a memory barrier
(`…ACCELERATION_STRUCTURE_BUILD_BIT_KHR` → `…FRAGMENT_SHADER_BIT`) so the TLAS is
visible to ray queries in the fragment shader.

### Vulkan-specific methods

#### `VkAccelerationStructureKHR handle() const`

Returns the raw TLAS handle, written into the
`VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` descriptor by `vk::ResourceSet`.

---

## metal::RayTracingScene

**Header:** `<bg2e/gpu/metal/RayTracingScene.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::RayTracingScene`

```cpp
class RayTracingScene : public gpu::RayTracingScene {
public:
    RayTracingScene(metal::Device* device, const std::string& debugName = {});

    void buildOrUpdate(gpu::CommandBuffer* cmd) override;
    void cleanup() override;
    bool isValid() const override;

    void build(MTL::CommandBuffer* cmd);   // records the build on an acceleration structure encoder

    MTL::AccelerationStructure* handle() const;                                   // macOS only
    const std::vector<MTL::AccelerationStructure*>& referencedPrimitives() const; // macOS only
};
```

Metal instance acceleration structure. The build uses a
`MTL::InstanceAccelerationStructureDescriptor` plus an instance descriptor
buffer of `MTL::AccelerationStructureInstanceDescriptor` entries (transform as
`PackedFloat4x3`, options for opaque triangle geometry, mask, and the index of
the referenced primitive acceleration structure). Instance buffer, scratch
buffer and the acceleration structure are reused and grown on demand.

### Metal-specific methods

#### `MTL::AccelerationStructure* handle() const`

Returns the raw instance acceleration structure (macOS only). Bound via
`setFragmentAccelerationStructure` / `setVertexAccelerationStructure` at the
configured Metal buffer index.

#### `const std::vector<MTL::AccelerationStructure*>& referencedPrimitives() const`

Returns the primitive acceleration structures referenced by the current build.
They must be made resident (`useResource`) before the instance structure is used
in a shader; `metal::CommandBuffer::bindResourceSet` does this automatically.
