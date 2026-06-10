# ShaderModule

**Header:** `<bg2e/gpu/ShaderModule.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API ShaderModule {
public:
    virtual ~ShaderModule() = default;
    virtual ShaderStage stage() const = 0;
    virtual bool isValid() const = 0;
    virtual void cleanup() = 0;
};
```

Abstract shader module. Wraps a compiled shader (SPIR-V for Vulkan, metallib
for Metal). Created via `Device::createShaderModule()`.

---

## Methods

### `virtual ShaderStage stage() const = 0`

Returns the shader stage this module targets (`Vertex`, `Fragment`, or
`Compute`).

### `virtual bool isValid() const = 0`

Returns `true` if the shader module was successfully loaded and compiled.

### `virtual void cleanup() = 0`

Releases the shader module resources.

---

## ShaderModuleDescription

Defined in `<bg2e/gpu/Common.hpp>`:

```cpp
struct ShaderModuleDescription {
    std::string filePath;            // Vulkan: .spv ; Metal: .metallib
    std::string entryPoint = "main"; // Vulkan: SPIR-V entry ; Metal: MTL function name
    ShaderStage stage = ShaderStage::Vertex;
};
```

| Field        | Type          | Description                                    |
|--------------|---------------|------------------------------------------------|
| `filePath`   | `std::string` | Path to the compiled shader file.              |
| `entryPoint` | `std::string` | Entry point function name (default: `"main"`). |
| `stage`      | `ShaderStage` | Pipeline stage this shader targets.            |

---

## Example

```cpp
// Vulkan: load SPIR-V files
auto vs = device->createShaderModule({
    "shaders/triangle.vert.spv", "main", gpu::ShaderStage::Vertex
});
auto fs = device->createShaderModule({
    "shaders/triangle.frag.spv", "main", gpu::ShaderStage::Fragment
});

// Metal: load from metallib with named functions
auto vs = device->createShaderModule({
    "shaders/triangle.metallib", "triangle_vertex", gpu::ShaderStage::Vertex
});
auto fs = device->createShaderModule({
    "shaders/triangle.metallib", "triangle_fragment", gpu::ShaderStage::Fragment
});
```

---

## vk::ShaderModule

**Header:** `<bg2e/gpu/vk/ShaderModule.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::ShaderModule`

```cpp
class ShaderModule : public gpu::ShaderModule {
public:
    ShaderModule(VkDevice device, const gpu::ShaderModuleDescription& description);
    ~ShaderModule() override;

    gpu::ShaderStage stage() const override;
    bool isValid() const override;
    void cleanup() override;

    VkShaderModule handle() const;
    const std::string& entryPoint() const;
};
```

### Vulkan-specific methods

#### `VkShaderModule handle() const`

Returns the raw `VkShaderModule` handle.

#### `const std::string& entryPoint() const`

Returns the entry point name.

---

## metal::ShaderModule

**Header:** `<bg2e/gpu/metal/ShaderModule.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::ShaderModule`

Metal shader module. Loads a `.metallib` file and extracts the named function
for the specified stage.
