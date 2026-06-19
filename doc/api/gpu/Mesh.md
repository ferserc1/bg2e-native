# Mesh

**Header:** `<bg2e/gpu/Mesh.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
template <typename MeshT>
class MeshGeneric {
public:
    using geo_type = MeshT;

    MeshT&       meshData();
    const MeshT& meshData() const;
    void setMeshData(const MeshT& data);
    void setMeshData(const MeshT* data);

    uint32_t submeshCount() const;

    void build(gpu::Device* device);
    void draw(gpu::CommandBuffer* cmd);
    void drawSubmesh(gpu::CommandBuffer* cmd, uint32_t submeshIndex);

    static gpu::VertexBufferDescription vertexBufferDescription();

    gpu::RayTracingMeshDescription rayTracingMeshDescription(uint32_t submeshIndex) const;

    gpu::Buffer*       vertexBuffer();
    const gpu::Buffer* vertexBuffer() const;
    gpu::Buffer*       indexBuffer();
    const gpu::Buffer* indexBuffer() const;

    void cleanup();
};
```

Template wrapper that pairs a CPU-side `bg2e::geo::MeshGeneric<VertexT>`
with two GPU `Buffer` objects (one for vertices, one for indices). Provides
a one-stop `build` / `draw` / `cleanup` API.

---

## Type aliases

All standard geo vertex types have a corresponding `gpu` alias:

| GPU type        | CPU type (`bg2e::geo::`) | Vertex fields                              |
|-----------------|--------------------------|--------------------------------------------|
| `gpu::MeshP`    | `MeshP`                  | position                                   |
| `gpu::MeshPN`   | `MeshPN`                 | position, normal                           |
| `gpu::MeshPC`   | `MeshPC`                 | position, color                            |
| `gpu::MeshPU`   | `MeshPU`                 | position, texCoord0                        |
| `gpu::MeshPNU`  | `MeshPNU`                | position, normal, texCoord0                |
| `gpu::MeshPNC`  | `MeshPNC`                | position, normal, color                    |
| `gpu::MeshPNUC` | `MeshPNUC`               | position, normal, texCoord0, color         |
| `gpu::MeshPNUT` | `MeshPNUT`               | position, normal, texCoord0, tangent       |
| `gpu::MeshPNUUT`| `MeshPNUUT`              | position, normal, texCoord0, texCoord1, tangent |
| `gpu::Mesh`     | `Mesh` (= `MeshPNUUT`)   | All standard fields (default mesh type)    |

---

## Methods

### `MeshT& meshData()` / `const MeshT& meshData() const`

Returns a reference to the underlying CPU mesh data (vertices, indices,
submeshes).

### `void setMeshData(const MeshT& data)` / `void setMeshData(const MeshT* data)`

Replaces the CPU mesh data. Call `build()` afterward to re-upload to the GPU.

### `uint32_t submeshCount() const`

Returns the number of submeshes (`meshData().submeshes.size()`).

### `void build(gpu::Device* device)`

Allocates and uploads the vertex and index buffers to the GPU. Must be called
once before any `draw` calls. Internally calls:

```
device->createBuffer() -> buf->createVertexBuffer(meshData().vertices)
device->createBuffer() -> buf->createIndexBuffer(meshData().indices)
```

### `void draw(gpu::CommandBuffer* cmd)`

Records the full draw sequence for all submeshes:

1. `cmd->bindVertexBuffer(0, vertexBuffer, 0)`
2. `cmd->bindIndexBuffer(indexBuffer, 0)`
3. For each submesh: `cmd->drawIndexed(submesh.indexCount, 1, submesh.firstIndex, 0, 0)`

### `void drawSubmesh(gpu::CommandBuffer* cmd, uint32_t submeshIndex)`

Records a `drawIndexed` call for a single submesh. Vertex and index buffers
must already be bound (call `draw()` to bind everything automatically).

### `static gpu::VertexBufferDescription vertexBufferDescription()`

Returns the `VertexBufferDescription` that matches the vertex layout of this
mesh type. Pass this to `GraphicsPipelineDescription::addVertexBufferDescription`
when creating a pipeline that will render this mesh type.

```cpp
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
```

### `gpu::RayTracingMeshDescription rayTracingMeshDescription(uint32_t submeshIndex) const`

Builds a [`RayTracingMeshDescription`](RayTracingMesh.md) for one submesh,
**reusing** the vertex and index buffers already created by `build()`. The
vertex stride and position offset are resolved from this mesh type's vertex
layout. Throws `std::runtime_error` if `submeshIndex` is out of range.

Use it to create a [`RayTracingMesh`](RayTracingMesh.md) per submesh without
duplicating any GPU buffer:

```cpp
for (uint32_t s = 0; s < mesh.submeshCount(); ++s) {
    auto rtMesh = device->createRayTracingMesh(mesh.rayTracingMeshDescription(s));
}
```

### `gpu::Buffer* vertexBuffer()` / `gpu::Buffer* indexBuffer()`

Returns the underlying GPU buffer (or `nullptr` before `build()` is called).

### `void cleanup()`

Releases both GPU buffers. Must be called before `Device::cleanup()`.

---

## Vertex attribute locations

Each mesh type exposes a fixed set of shader `layout(location = N) in` inputs.
The location assignments are the same across all mesh types:

| Location | Semantic      | Format                | Vertex type field |
|----------|---------------|-----------------------|-------------------|
| 0        | Position      | `R32G32B32_SFLOAT`    | `position`        |
| 1        | Normal        | `R32G32B32_SFLOAT`    | `normal`          |
| 1        | TexCoord0 *   | `R32G32_SFLOAT`       | `texCoord0`       |
| 1        | Color *       | `R32G32B32A32_SFLOAT` | `color`           |
| 2        | TexCoord0     | `R32G32_SFLOAT`       | `texCoord0`       |
| 3        | TexCoord1     | `R32G32_SFLOAT`       | `texCoord1`       |
| 3        | Tangent *     | `R32G32B32_SFLOAT`    | `tangent`         |
| 4        | Tangent       | `R32G32B32_SFLOAT`    | `tangent`         |

\* Location depends on which fields are present in the specific type (consult
`vertexBufferDescription()` for the exact layout).

The simplest types:

| Type       | loc 0    | loc 1      |
|------------|----------|------------|
| `MeshP`    | position | –          |
| `MeshPU`   | position | texCoord0  |
| `MeshPN`   | position | normal     |

---

## Example

```cpp
// 1. Build CPU mesh
bg2e::geo::MeshPU meshData;
meshData.vertices = {
    { { 0.0f, -0.5f, 0.0f }, { 0.5f, 0.0f } },
    { { 0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f } },
    { {-0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f } },
};
meshData.indices  = { 0, 1, 2 };
meshData.submeshes = { { 0, 3 } };

// 2. Wrap in a GPU mesh and upload
gpu::MeshPU mesh;
mesh.setMeshData(meshData);
mesh.build(device.get());

// 3. Configure pipeline with matching vertex layout
gpu::GraphicsPipelineDescription pipelineDesc{};
// ... fill other fields ...
pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
auto pipeline = device->createGraphicsPipeline(pipelineDesc);

// 4. Draw each frame
cmd->bindPipeline(pipeline.get());
mesh.draw(cmd.get());

// 5. Cleanup (before device->cleanup())
device->waitIdle();
mesh.cleanup();
```

### Matching GLSL vertex shader (`MeshPU`)

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

### Matching Metal vertex shader (`MeshPU`)

```metal
struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};
struct VertexOut { float4 position [[position]]; float2 uv; };

vertex VertexOut triangle_vertex(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.uv = in.texCoord;
    return out;
}
```
