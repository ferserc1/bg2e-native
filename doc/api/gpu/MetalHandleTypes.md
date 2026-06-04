# Metal Handle Types

**Header:** `<bg2e/gpu/metal/common.hpp>`
**Namespace:** `bg2e::gpu::metal`

Type aliases for Metal framework objects. On macOS, these map to the actual
Metal/MetalCPP types. On other platforms, opaque stubs are provided so that
Metal backend code compiles everywhere.

---

## On macOS (`BG2E_IS_MAC`)

```cpp
using DeviceHandle       = MTL::Device*;
using CommandQueueHandle = MTL::CommandQueue*;
using MetalLayerHandle   = CA::MetalLayer*;
```

| Alias                | Type              | Description                        |
|----------------------|-------------------|------------------------------------|
| `DeviceHandle`       | `MTL::Device*`    | Metal GPU device.                  |
| `CommandQueueHandle` | `MTL::CommandQueue*` | Metal command queue.            |
| `MetalLayerHandle`   | `CA::MetalLayer*` | Core Animation Metal layer.        |

---

## On non-macOS platforms

```cpp
struct DeviceOpaque;
using DeviceHandle = DeviceOpaque*;

struct CommandQueueOpaque;
using CommandQueueHandle = CommandQueueOpaque*;

struct MetalLayerOpaque;
using MetalLayerHandle = MetalLayerOpaque*;
```

Opaque pointer types that allow Metal backend code to compile on Linux and
Windows without linking against the Metal framework.
