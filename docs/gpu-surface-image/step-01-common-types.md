# Step 1 — Common types: `Size2D`, `Size3D`, `PixelFormat`

**Goal:** Add the shared value types used across the rest of the plan. Purely additive — no
existing API changes, so everything keeps compiling and nothing references them yet.

## Files

- **Modify** `lib/include/bg2e/gpu/Common.hpp`

## `Common.hpp` additions

Add inside `namespace bg2e { namespace gpu {` (after the existing `BackendType` enum):

```cpp
#include <cstdint>   // ensure included

struct Size2D {
    uint32_t width  = 0;
    uint32_t height = 0;

    Size2D() = default;
    Size2D(uint32_t w, uint32_t h) : width(w), height(h) {}

    bool operator==(const Size2D& o) const { return width == o.width && height == o.height; }
    bool operator!=(const Size2D& o) const { return !(*this == o); }
    bool isZero() const { return width == 0 || height == 0; }
};

struct Size3D {
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t depth  = 1;

    Size3D() = default;
    Size3D(uint32_t w, uint32_t h, uint32_t d = 1) : width(w), height(h), depth(d) {}
    explicit Size3D(const Size2D& s, uint32_t d = 1) : width(s.width), height(s.height), depth(d) {}

    Size2D toSize2D() const { return Size2D{ width, height }; }

    bool operator==(const Size3D& o) const { return width == o.width && height == o.height && depth == o.depth; }
    bool operator!=(const Size3D& o) const { return !(*this == o); }
};
```

### `PixelFormat` enum

Backend-agnostic format list covering the common color and depth/stencil formats shared by
Vulkan and Metal. Naming follows the `R8G8B8A8_UNORM` style requested.

```cpp
enum class PixelFormat {
    Undefined = 0,

    // --- Color ---
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_SFLOAT,
    R32G32B32A32_SFLOAT,

    // --- Depth / stencil ---
    D16_UNORM,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT
};

// Small helpers (constexpr, header-only) — useful later for surfaces/images.
constexpr bool isDepthFormat(PixelFormat f)
{
    return f == PixelFormat::D16_UNORM
        || f == PixelFormat::D32_SFLOAT
        || f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

constexpr bool hasStencil(PixelFormat f)
{
    return f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}
```

## Notes

- Keep everything header-only and `constexpr` where possible; no `.cpp` needed.
- `Size2D` will become the canonical size type in Step 3; `Size3D` is provided now for future
  3D/volume images even though Step 5/6 images are 2D.
- The `D24_UNORM_S8_UINT` format is not universally supported on Apple GPUs — Step 2 documents
  the Metal mapping caveat; selection/fallback logic is out of scope here.

## Compile check

No existing declarations change; the new symbols are unreferenced. Build stays green.
