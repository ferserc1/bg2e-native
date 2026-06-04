# OffscreenSurface

**Header:** `<bg2e/gpu/OffscreenSurface.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API OffscreenSurface : public Surface {
public:
    OffscreenSurface(uint32_t width, uint32_t height);

    bool isOffscreen() const override { return true; }
    bool isValid()     const override { return true; }
    uint32_t width()   const override;
    uint32_t height()  const override;
};
```

Concrete surface for headless/offscreen rendering. No `create()` call is
needed; the surface is valid immediately after construction.

---

## Constructor

### `OffscreenSurface(uint32_t width, uint32_t height)`

| Parameter | Type       | Description               |
|-----------|------------|---------------------------|
| `width`   | `uint32_t` | Surface width in pixels.  |
| `height`  | `uint32_t` | Surface height in pixels. |

---

## Methods

### `bool isOffscreen() const override`

Always returns `true`.

### `bool isValid() const override`

Always returns `true` (offscreen surfaces are always valid).

### `uint32_t width() const override`

Returns the width set in the constructor.

### `uint32_t height() const override`

Returns the height set in the constructor.

---

## vk::OffscreenSurface

**Header:** `<bg2e/gpu/vk/OffscreenSurface.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::OffscreenSurface`

```cpp
class OffscreenSurface : public gpu::OffscreenSurface {
public:
    OffscreenSurface(uint32_t width, uint32_t height)
        : gpu::OffscreenSurface(width, height) {}
};
```

Thin Vulkan subclass. No additional methods or properties.

---

## metal::OffscreenSurface

**Header:** `<bg2e/gpu/metal/OffscreenSurface.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::OffscreenSurface`

```cpp
class OffscreenSurface : public gpu::OffscreenSurface {
public:
    OffscreenSurface(uint32_t width, uint32_t height)
        : gpu::OffscreenSurface(width, height) {}
};
```

Thin Metal subclass. No additional methods or properties.
