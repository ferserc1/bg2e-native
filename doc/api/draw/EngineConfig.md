# EngineConfig

**Header:** `<bg2e/draw/EngineConfig.hpp>`  
**Namespace:** `bg2e::draw`

An aggregate configuration for experimental draw execution. The backend field
selects the low-level GPU API; the MainLoop run overload selects the high-level
framework.

| Field | Type | Default |
|-------|------|---------|
| `backend` | `gpu::BackendType` | Metal on macOS; Vulkan elsewhere |
| `debug` | `bool` | `false` |
| `applicationName` | `std::string` | Empty string |
| `colorFormat` | `gpu::PixelFormat` | `B8G8R8A8_UNORM` |
| `depthFormat` | `gpu::PixelFormat` | `D32_SFLOAT` |

The backend default uses the platform macros from `base/PlatformTools.hpp`.
No Metal framework types or headers are needed by this configuration.

```cpp
bg2e::draw::EngineConfig config;
// Explicit Vulkan remains available on macOS.
config.backend = bg2e::gpu::BackendType::Vulkan;
```

MainLoop copies the configuration. Metal selected outside macOS is rejected
with `std::invalid_argument` before allocation. There is no runtime automatic
fallback to another backend. The empty application name reserves the MainLoop
appId fallback for GPU initialization; the milestone 01 runtime boundary occurs
before that initialization, so no name is applied to a GPU instance yet.

Configuration fields do not make runtime execution available: a valid
experimental run still throws the [milestone 01 availability exception](../app/MainLoop.md#experimental-runtime-boundary).
