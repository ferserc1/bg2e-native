# Step 1 — Value types (`Color`, `ImageLayout`) + `Image` layout state

**Goal:** add the shared value types used by the loop and give `gpu::Image` the ability to track
its own current layout. Purely additive: no existing signature changes, nothing references the new
types yet, so the build stays green on every backend.

## Files

- **Modify** `lib/include/bg2e/gpu/Common.hpp`
- **Modify** `lib/include/bg2e/gpu/Image.hpp`

## `Common.hpp` additions

Add inside `namespace bg2e { namespace gpu {`, after the existing `PixelFormat` helpers.

### `gpu::Color`

The sketch builds `gpu::Color { r, g, b, a }` with aggregate-style initialization. Keep it a
plain, backend-agnostic POD (do **not** reuse `bg2e::base::Color`; the `gpu` layer must stay
self-contained at this level).

```cpp
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};
```

### `gpu::ImageLayout`

Backend-agnostic logical layouts. Only the values used by the loop are required now; the extra
ones make the abstraction usable later without another migration.

```cpp
enum class ImageLayout {
    Undefined = 0,    // initial / unknown state
    General,          // generic read/write
    ColorAttachment,  // being rendered to as color
    DepthAttachment,  // being rendered to as depth (/stencil)
    ShaderReadOnly,   // sampled in a shader
    TransferSrc,      // copy source
    TransferDst,      // copy destination
    Present           // ready for presentation (window surface)
};
```

## `Image.hpp` additions

`gpu::Image` gains an internal current-layout field plus a public getter and a setter usable by
the command buffers. Per the design note, `CommandBuffer` is granted friendship so it can read and
update the layout during `transition()`.

Add the forward declarations and friend grants, the new member, the getter and the setter:

```cpp
namespace bg2e {
namespace gpu {

namespace vk    { class CommandBuffer; }
namespace metal { class CommandBuffer; }

class BG2E_API Image {
public:
    virtual ~Image() = default;

    virtual void cleanup() = 0;
    virtual bool isValid() const = 0;

    PixelFormat   pixelFormat() const { return _pixelFormat; }
    const Size2D& size()        const { return _size; }
    uint32_t      width()       const { return _size.width;  }
    uint32_t      height()      const { return _size.height; }

    ImageLayout   currentLayout() const { return _currentLayout; }

protected:
    void setCurrentLayout(ImageLayout layout) { _currentLayout = layout; }

    PixelFormat _pixelFormat   = PixelFormat::Undefined;
    Size2D      _size;
    ImageLayout _currentLayout = ImageLayout::Undefined;

    friend class vk::CommandBuffer;
    friend class metal::CommandBuffer;
};

}
}
```

Notes:

- `setCurrentLayout` is `protected`; the `friend` grants let `vk::CommandBuffer` /
  `metal::CommandBuffer` call it on any `gpu::Image*`. Backend subclasses (`vk::Image`,
  `metal::Image`) can also set it through normal inheritance when they (re)create a resource.
- `_currentLayout` defaults to `Undefined`. Wrapped swapchain/drawable images are re-created each
  frame and naturally start `Undefined`, which is the correct source layout for the first
  transition of the frame (Vulkan can transition from `UNDEFINED` discarding contents).

## Optional (recommended) — reset layout on (re)build

To keep the tracked state honest, when a `vk::Image` / `metal::Image` (re)creates its underlying
resource it should reset the layout to `Undefined`. This is a one-line addition inside the
existing build/init methods and is safe (those methods already exist):

- `vk::Image::buildTargetImage / buildDepthImage / initFromSwapchainImage`: set
  `_currentLayout = ImageLayout::Undefined;` after assigning `_size` / `_pixelFormat`.
- `metal::Image::buildTargetImage / buildDepthImage`: same.

This is optional for compilation but avoids stale layout assumptions after a resize.

## Build check

Header-only additions plus optional one-liners in existing `.cpp` build methods. No call sites
change. `gpu/all.hpp` already includes `Common.hpp` and `Image.hpp`. Build stays green.
