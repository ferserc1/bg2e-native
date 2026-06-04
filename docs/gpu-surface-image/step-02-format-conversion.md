# Step 2 — Pixel format conversion (Vulkan & Metal)

**Goal:** Provide the backend-specific equivalence between `gpu::PixelFormat` and the native
format enums (`VkFormat`, `MTL::PixelFormat`), in both directions. Standalone helper functions
that nothing calls yet → build stays green.

Bidirectional mapping is required because:
- `to<Backend>Format()` is used when **creating** images/swapchains from a `PixelFormat`.
- `from<Backend>Format()` is used when **wrapping** existing resources (e.g. the swapchain
  picks a `VkFormat`; the surface must report the matching `PixelFormat`).

## Files

Two options — pick one (the plan assumes **Option A**):

- **Option A (fold into existing common headers):**
  - Modify `lib/include/bg2e/gpu/vk/common.hpp` (+ a new `lib/src/bg2e/gpu/vk/Format.cpp` or inline)
  - Modify `lib/include/bg2e/gpu/metal/common.hpp` (+ `lib/src/bg2e/gpu/metal/Format.cpp`)
- **Option B (dedicated headers):** create `vk/Format.hpp` / `metal/Format.hpp`.

Implementation note: keep the mapping in `.cpp` files (a `switch`) to avoid pulling heavy
headers into widely-included `common.hpp`; only the declarations live in the header.

## Vulkan mapping

Declarations (header):

```cpp
namespace bg2e::gpu::vk {
    VkFormat    toVkFormat(gpu::PixelFormat format);
    PixelFormat fromVkFormat(VkFormat format);
}
```

Definition (`.cpp`) — exhaustive `switch`:

| `gpu::PixelFormat`     | `VkFormat` |
|------------------------|------------|
| `R8G8B8A8_UNORM`       | `VK_FORMAT_R8G8B8A8_UNORM` |
| `R8G8B8A8_SRGB`        | `VK_FORMAT_R8G8B8A8_SRGB` |
| `B8G8R8A8_UNORM`       | `VK_FORMAT_B8G8R8A8_UNORM` |
| `B8G8R8A8_SRGB`        | `VK_FORMAT_B8G8R8A8_SRGB` |
| `R16G16B16A16_SFLOAT`  | `VK_FORMAT_R16G16B16A16_SFLOAT` |
| `R32G32B32A32_SFLOAT`  | `VK_FORMAT_R32G32B32A32_SFLOAT` |
| `D16_UNORM`            | `VK_FORMAT_D16_UNORM` |
| `D32_SFLOAT`           | `VK_FORMAT_D32_SFLOAT` |
| `D24_UNORM_S8_UINT`    | `VK_FORMAT_D24_UNORM_S8_UINT` |
| `D32_SFLOAT_S8_UINT`   | `VK_FORMAT_D32_SFLOAT_S8_UINT` |
| `Undefined`            | `VK_FORMAT_UNDEFINED` |

`fromVkFormat` is the inverse `switch`; unknown values → `PixelFormat::Undefined`.

## Metal mapping

Metal's `MTL::PixelFormat` only exists on macOS, so all definitions go inside
`#if BG2E_IS_MAC`. On non-mac the functions are still declared (so other gpu code can compile),
but their definitions are `#else`-guarded to throw / return `Undefined`.

Declarations (header):

```cpp
namespace bg2e::gpu::metal {
#if BG2E_IS_MAC
    MTL::PixelFormat toMetalPixelFormat(gpu::PixelFormat format);
    PixelFormat      fromMetalPixelFormat(MTL::PixelFormat format);
#endif
}
```

Definition (`.cpp`, inside `#if BG2E_IS_MAC`):

| `gpu::PixelFormat`     | `MTL::PixelFormat` |
|------------------------|--------------------|
| `R8G8B8A8_UNORM`       | `MTL::PixelFormatRGBA8Unorm` |
| `R8G8B8A8_SRGB`        | `MTL::PixelFormatRGBA8Unorm_sRGB` |
| `B8G8R8A8_UNORM`       | `MTL::PixelFormatBGRA8Unorm` |
| `B8G8R8A8_SRGB`        | `MTL::PixelFormatBGRA8Unorm_sRGB` |
| `R16G16B16A16_SFLOAT`  | `MTL::PixelFormatRGBA16Float` |
| `R32G32B32A32_SFLOAT`  | `MTL::PixelFormatRGBA32Float` |
| `D16_UNORM`            | `MTL::PixelFormatDepth16Unorm` |
| `D32_SFLOAT`           | `MTL::PixelFormatDepth32Float` |
| `D24_UNORM_S8_UINT`    | `MTL::PixelFormatDepth24Unorm_Stencil8` *(see caveat)* |
| `D32_SFLOAT_S8_UINT`   | `MTL::PixelFormatDepth32Float_Stencil8` |
| `Undefined`            | `MTL::PixelFormatInvalid` |

`fromMetalPixelFormat` is the inverse; unknown → `PixelFormat::Undefined`.

### Caveats
- `MTL::PixelFormatDepth24Unorm_Stencil8` is only available on macOS discrete GPUs
  (`depth24Stencil8PixelFormatSupported`). For Apple-silicon targets prefer
  `D32_SFLOAT` / `D32_SFLOAT_S8_UINT`. Runtime capability checking/fallback is **out of scope**
  for this step; document the limitation and let callers pick a supported depth format.

## Compile check

Functions are defined but unreferenced. The Metal `.cpp` compiles on all platforms thanks to
the `#if BG2E_IS_MAC` guard. Build stays green.
