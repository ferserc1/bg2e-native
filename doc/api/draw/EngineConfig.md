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
fallback to another backend. An empty application name is replaced by MainLoop's
appId during backend preparation, then applied during Instance initialization.
The requested color/depth formats configure the surface; the coordinator uses
the actual acquired image extent/format and surface generation for retained color
and UI compatibility. The scene FrameContext is color-only even though
the surface may own depth resources.

See [framework scope and maturity](index.md#production-and-experimental-frameworks)
for the distinction between render and draw.
