# Step 004 — Base `gpu::Image::readPixelsRGBA8`

**Type:** Base API
**Depends on:** none
**Enables:** 005 (Vulkan), 006 (Metal)

## Goal

Add the backend-agnostic read-back entry point to `gpu::Image`, following the compile-safe
extension pattern: a **non-pure virtual** with a default implementation that throws
"not implemented", so the base step compiles with both backends inheriting the default, and
each backend overrides it in a later step.

## API (`lib/include/bg2e/gpu/Image.hpp`)

Add the include and the method:

```cpp
#include <vector>
#include <stdexcept>
// ...
class BG2E_API Image {
public:
    // ...
    // Reads the image contents into a tightly packed RGBA8 buffer (4 bytes/pixel,
    // row-major, top-left origin). `currentLayout` is the layout the image is in at the
    // moment of the call (used by backends that need an explicit transition before/after
    // the copy). Only PixelFormat::R8G8B8A8_UNORM is supported initially.
    virtual void readPixelsRGBA8(
        std::vector<uint8_t>& outData,
        ImageLayout currentLayout = ImageLayout::ColorAttachment)
    {
        throw std::runtime_error("Image::readPixelsRGBA8 not implemented");
    }
    // ...
};
```

## Rationale

- `currentLayout` mirrors the stable `render::vulkan::Image::readPixelsRGBA8` signature, which
  takes the initial layout so it can transition to `TRANSFER_SRC` and back. Metal ignores it
  (no explicit layouts) but accepts it for a symmetric API. Default `ColorAttachment` matches
  the layout an offscreen color target is typically left in after rendering; the example will
  pass whatever final layout it transitioned the image to.
- The output is always tightly packed `width * height * 4` bytes, so callers can hand
  `outData.data()` straight to `bg2e::db::saveImage(path, data, width, height, 4)`.
- No device argument: each backend `Image` already stores its `Device` (`vk::Image::_device`,
  `metal::Image::_device`), which is enough to run the read-back.

## Validation

- `bg2e` and all examples build on all platforms (both backends inherit the throwing default;
  nothing calls it yet).
