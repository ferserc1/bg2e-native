# PhysicalDeviceProperties

**Header:** `<bg2e/gpu/PhysicalDevice.hpp>`
**Namespace:** `bg2e::gpu`

## RayTracingCapabilities

```cpp
struct RayTracingCapabilities {
    bool available = false;
    bool rayTracingPipeline = false;
    bool rayQuery = false;
    bool accelerationStructure = false;
    bool bufferDeviceAddress = false;

    inline bool fullSupported() const;
};
```

| Field                   | Type   | Default | Description                                    |
|-------------------------|--------|---------|------------------------------------------------|
| `available`             | `bool` | `false` | General ray tracing support reported by driver.|
| `rayTracingPipeline`    | `bool` | `false` | Hardware ray tracing pipeline support.         |
| `rayQuery`              | `bool` | `false` | Ray queries callable from raster shaders.      |
| `accelerationStructure` | `bool` | `false` | BVH acceleration structure support.            |
| `bufferDeviceAddress`   | `bool` | `false` | Buffer device address (needed for AS buffers). |

### `bool fullSupported() const`

Returns `true` if and only if all five fields are `true`.

---

## DeviceType

```cpp
enum DeviceType {
    IntegratedGPU,
    DiscreteGPU,
    VirtualGPU,
    CPU,
    Other
};
```

| Value           | Description                        |
|-----------------|------------------------------------|
| `IntegratedGPU` | GPU integrated into the CPU die.   |
| `DiscreteGPU`   | Dedicated GPU on a separate chip.  |
| `VirtualGPU`    | Virtual or software-emulated GPU.  |
| `CPU`           | CPU-based rendering device.        |
| `Other`         | Unknown or unclassified device.    |

---

## PhysicalDeviceProperties

```cpp
struct PhysicalDeviceProperties {
    enum DeviceType { ... } deviceType = Other;
    size_t totalHeapMemoryMB = 0;
    DeviceType type = Other;
    std::string name;
    uint32_t vendor = 0;
    uint32_t id = 0;
    RayTracingCapabilities rayTracing;

    uint32_t getScore() const;
    bool rayTracingSupported() const;

    static PhysicalDeviceProperties* create();

protected:
    PhysicalDeviceProperties() = default;
};
```

Stores queried properties of a GPU device. The constructor is protected;
instances are created via the static `create()` factory method.

| Field               | Type                     | Default   | Description                        |
|---------------------|--------------------------|-----------|------------------------------------|
| `deviceType`        | `DeviceType`             | `Other`   | Classification of the GPU.         |
| `totalHeapMemoryMB` | `size_t`                 | `0`       | Total GPU memory in megabytes.     |
| `type`              | `DeviceType`             | `Other`   | Duplicate of `deviceType`.         |
| `name`              | `std::string`            | `""`      | Human-readable GPU name.           |
| `vendor`            | `uint32_t`               | `0`       | PCI vendor ID.                     |
| `id`                | `uint32_t`               | `0`       | PCI device ID.                     |
| `rayTracing`        | `RayTracingCapabilities` | (defaults)| Ray tracing feature flags.         |

### `uint32_t getScore() const`

Computes a ranking score for automatic device selection:

1. Base score = `totalHeapMemoryMB`
2. Multiply by device type factor:
   - DiscreteGPU: x100
   - IntegratedGPU: x10
   - VirtualGPU: x5
   - CPU: +1
3. If `rayTracing.fullSupported()` is true, multiply by 100

Higher scores are preferred by `PhysicalDevice::choose()`.

### `bool rayTracingSupported() const`

Returns `true` if `rayTracing.fullSupported()` is true.

### `static PhysicalDeviceProperties* create()`

Factory method. Returns a heap-allocated instance. The caller takes ownership.
