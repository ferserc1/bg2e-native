# Step 006 — `metal::Image::readPixelsRGBA8`

**Type:** Backend (Metal, macOS only)
**Depends on:** 003 (`gpu::Device::immediateSubmit`, Metal override), 004 (base API)
**Enables:** 007 (example, Metal path)

## Goal

Implement the Metal GPU→CPU read-back: blit the private texture into a host-visible
`MTL::Buffer` (`StorageModeShared`) via the backend-agnostic `gpu::Device::immediateSubmit`,
then `memcpy` from `buffer->contents()` into the output vector, removing any row padding the
blit required. Guarded by `BG2E_IS_MAC`; a throwing stub elsewhere.

Metal has no explicit image layouts, so the closure does not record any `cmd->transition(...)`;
it only down-casts the abstract `gpu::CommandBuffer*` to reach the native `MTL::CommandBuffer*`
and create a blit encoder.

## API (`lib/include/bg2e/gpu/metal/Image.hpp`)

```cpp
void readPixelsRGBA8(std::vector<uint8_t>& outData,
                     ImageLayout currentLayout = ImageLayout::ColorAttachment) override;
```

(`currentLayout` is accepted for API symmetry and ignored — Metal has no explicit image
layouts.)

## Implementation (`lib/src/bg2e/gpu/metal/Image.cpp`, `#if BG2E_IS_MAC`)

```cpp
void Image::readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout /*currentLayout*/)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("metal::Image::readPixelsRGBA8: only R8G8B8A8_UNORM is supported");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;

    // Blit destinationBytesPerRow must be 256-byte aligned on macOS for buffer copies.
    const uint32_t unpaddedBytesPerRow = w * 4;
    const uint32_t alignment = 256;
    const uint32_t paddedBytesPerRow =
        ((unpaddedBytesPerRow + alignment - 1) / alignment) * alignment;
    const size_t   bufferSize = size_t(paddedBytesPerRow) * h;

    MTL::Buffer* staging = _device->handle()->newBuffer(
        bufferSize, MTL::ResourceStorageModeShared);
    if (!staging) {
        throw std::runtime_error("metal::Image::readPixelsRGBA8: newBuffer failed");
    }

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
        MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
        blit->copyFromTexture(
            _texture,                                  // sourceTexture
            0,                                         // sourceSlice
            0,                                         // sourceLevel
            MTL::Origin{ 0, 0, 0 },                    // sourceOrigin
            MTL::Size{ w, h, 1 },                      // sourceSize
            staging,                                   // destinationBuffer
            0,                                         // destinationOffset
            paddedBytesPerRow,                         // destinationBytesPerRow
            paddedBytesPerRow * h);                    // destinationBytesPerImage
        blit->endEncoding();
    });

    // Copy out, dropping per-row padding.
    outData.resize(size_t(unpaddedBytesPerRow) * h);
    const uint8_t* src = static_cast<const uint8_t*>(staging->contents());
    for (uint32_t y = 0; y < h; ++y) {
        std::memcpy(outData.data() + size_t(y) * unpaddedBytesPerRow,
                    src + size_t(y) * paddedBytesPerRow,
                    unpaddedBytesPerRow);
    }

    staging->release();
}
```

The `#else` branch adds the throwing stub (matching the file's existing structure):

```cpp
void Image::readPixelsRGBA8(std::vector<uint8_t>&, ImageLayout)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
```

## Notes / pitfalls

- **Row alignment** is the subtle part: `copyFromTexture:toBuffer:` requires
  `destinationBytesPerRow` to be a multiple of 256 on macOS (non–Apple-silicon GPUs in
  particular). Allocate the padded buffer, then strip the padding when copying into
  `outData`. This is why the read-back was flagged as the costliest part of the plan.
- The offscreen color texture is created with `StorageModePrivate` +
  `TextureUsageRenderTarget | TextureUsageShaderRead`. A blit **source** does not need an
  extra usage flag, so `metal::Image::buildTargetImage` needs no change. (If a future format
  requires it, add `MTL::TextureUsageShaderRead` is already present.)
- Pixel ordering: the offscreen surface uses `R8G8B8A8_UNORM`, which matches the byte order
  `db::saveImage` expects (R,G,B,A). No swizzle. If a BGRA texture is ever read back, a channel
  swap would be required — out of scope here.
- `_texture` and `_device` are already members; `_device` is a `metal::Device*`, whose
  `immediateSubmit` override (step 003) is reached through the abstract `gpu::Device` method.
- Includes needed in `Image.cpp`: `<bg2e/gpu/CommandBuffer.hpp>`,
  `<bg2e/gpu/metal/CommandBuffer.hpp>` (for the `handle()` down-cast), and `<cstring>` for
  `memcpy`.

## Validation

- On macOS, `bg2e` builds and links; the Metal run of the example (step 007) writes a correct
  `out.jpg`.
- On Linux/Windows, the throwing stub keeps the translation unit compiling; the method is never
  reached because the Metal backend isn't selectable there.
