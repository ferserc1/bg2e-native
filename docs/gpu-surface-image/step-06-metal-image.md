# Step 6 — `metal::Image`

**Goal:** Metal implementation of `gpu::Image`, owning an `MTL::Texture*`. Supports
`buildTargetImage` (offscreen color) and `buildDepthImage` (depth texture for windowed +
offscreen), plus `resize` / `cleanup`. Mirrors the `vk::Image` surface area so Step 7 can treat
both uniformly.

Not yet referenced by surfaces → build stays green. Compiles on all platforms via
`#if BG2E_IS_MAC` guards.

## Files

- **Create** `lib/include/bg2e/gpu/metal/Image.hpp`, `lib/src/bg2e/gpu/metal/Image.cpp`
- (`gpu/Image.hpp` and `all.hpp` already updated in Step 5.)

## `metal/Image.hpp`

```cpp
#pragma once
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e { namespace gpu { namespace metal {

class Device;   // fwd

class Image : public gpu::Image {
public:
    Image() = default;
    ~Image() override { cleanup(); }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    // Allocate an offscreen color render-target texture.
    void buildTargetImage(metal::Device* device, const Size2D& size, PixelFormat format);

    // Allocate a depth/stencil render-target texture.
    void buildDepthImage(metal::Device* device, const Size2D& size, PixelFormat format);

    void resize(const Size2D& size);

    void cleanup() override;
    bool isValid() const override;

    // Metal-specific accessor (used later for attachments / presentation).
    TextureHandle texture() const { return _texture; }

private:
    metal::Device* _device  = nullptr;   // not owned
    TextureHandle  _texture = nullptr;
    bool           _isDepth = false;     // remembered for resize
};

}}}
```

> Add `using TextureHandle = MTL::Texture*;` to `metal/common.hpp` inside the `#if BG2E_IS_MAC`
> block, and a matching opaque alias (`struct TextureOpaque; using TextureHandle = TextureOpaque*;`)
> in the `#else` block — same pattern already used there for `DeviceHandle` / `MetalLayerHandle`.

## `metal/Image.cpp` — behaviour

Wrap the whole implementation body in `#if BG2E_IS_MAC … #else …(stubs)… #endif`, like the
existing `metal/Device.cpp` / `metal/WindowSurface.cpp`.

Includes (mac branch): `<bg2e/gpu/metal/Image.hpp>`, `<bg2e/gpu/metal/Device.hpp>`,
`<bg2e/gpu/metal/common.hpp>`, and the Step-2 metal format helper.

### `buildTargetImage(device, size, format)`
1. `cleanup()`. Store `_device`, `_size`, `_pixelFormat = format`, `_isDepth = false`.
2. Build a texture descriptor:
   ```cpp
   auto* desc = MTL::TextureDescriptor::alloc()->init();
   desc->setTextureType(MTL::TextureType2D);
   desc->setPixelFormat(toMetalPixelFormat(format));
   desc->setWidth(size.width);
   desc->setHeight(size.height);
   desc->setStorageMode(MTL::StorageModePrivate);
   desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
   _texture = device->handle()->newTexture(desc);
   desc->release();
   if (!_texture) throw std::runtime_error("metal::Image::buildTargetImage: newTexture failed");
   ```

### `buildDepthImage(device, size, format)`
Same as above but:
- `_isDepth = true`.
- `desc->setUsage(MTL::TextureUsageRenderTarget)` (add `| MTL::TextureUsageShaderRead` if the
  depth texture will be sampled later).
- `setStorageMode(MTL::StorageModePrivate)` (depth attachments are device-private;
  use `StorageModeMemoryless` only for transient depth — out of scope, keep Private).

### `resize(size)`
Remember `_device`, `_pixelFormat`, `_isDepth`; `cleanup()`; then call
`buildTargetImage` or `buildDepthImage` based on `_isDepth`.

### `cleanup()`
```cpp
if (_texture) { _texture->release(); _texture = nullptr; }
```

### `isValid()`
`return _texture != nullptr;` (mac) / `return false;` (non-mac stub).

### Non-mac stubs
All methods throw `std::runtime_error("Metal backend is not available on this platform")`
(for build/resize) or no-op (`cleanup`), and `isValid()` returns `false`, matching the existing
metal stub convention.

## Compile check

New class, guarded for non-mac, unreferenced by surfaces. Build stays green on every platform.
