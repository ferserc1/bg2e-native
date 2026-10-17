# BlueNoise

**Header:** `<bg2e/utils/BlueNoise.hpp>` (also `<bg2e/utils/all.hpp>`)  
**Namespace:** `bg2e::utils`

`BlueNoiseData` is a borrowed CPU view of layer-major, row-major interleaved byte ranks, with `tileSize`, `channels`, `layers` and `bytes`. `embedded()` returns the existing immutable 128x128, two-channel, four-layer dataset. No graphics initialization or file IO is required. Custom data owners must outlive views; `validate()` requires exact dimensions/byte count, 1..4 channels and no size overflow. `texel(x,y,layer)` wraps signed coordinates periodically within a tile and rejects an invalid layer. Missing raw channels expand to zero, with alpha 255.

```cpp
const auto data = utils::BlueNoiseData::embedded();
const auto ranks = data.texel(-1, -1, 0);
const auto sample = data.sample2D(pixelX, pixelY, renderedFrame, sampleIndex);
const auto alternative = utils::sampleNoise2D(nullptr, pixelX, pixelY, renderedFrame, sampleIndex);
```

`sample2D` requires two source channels. It selects `(renderedFrame + sampleIndex*7) % layers`, maps byte ranks to bin centers `(rank+0.5)/256`, adds a fractional two-dimensional rotation and returns [0,1). Sequence arithmetic intentionally wraps uint32; rotation uses the low 16 bits of `renderedFrame*233 + sampleIndex*61` before float conversion. The embedded four-layer sample sequence repeats after 65536 rendered-frame keys for a fixed sample/pixel; this is not a new spatiotemporal noise distribution. Effects/bake jobs supply their own rendered-sample keys; utility sampling never advances hidden state or uses presentation/acquired-slot counters.

`sampleNoise2D(nullptr, ...)` selects a deterministic integer-hash alternative, also in [0,1). It is not blue noise. A non-null invalid data view throws rather than silently falling back.

## GPU residency and sharing

```cpp
// Stop producers before creation/replacement and keep Device alive.
auto noise = std::make_shared<utils::BlueNoise>(*device);
noise->build();
auto imageOwner = noise->image();
auto samplerOwner = noise->sampler();
auto metadata = noise->shaderMetadata();
```

The constructor borrows a gpu::Device. `build(data)` validates format/usage and dimensions, drains prior users, copies data to a new image and publishes image/sampler/metadata together after synchronous upload. A failure leaves the previous snapshot intact. There is no production Engine or native Vulkan type in this API, no RT support requirement and no global cache/per-frame texture ring. Custom byte owners may be released after build returns.

The GPU resource is a single RGBA8 UNORM color 2D atlas, width=tileWidth, height=tileHeight*logicalLayers, one mip. Source channels keep exact byte ranks; missing RGB channels are zero and missing alpha is 255. It supports sampling, upload and raw readback. The sampler is nearest/repeat with LOD clamped to zero. `shaderMetadata()` is a 16-byte uint4 `(tileWidth,tileHeight,sourceChannels,logicalLayers)`. `layerCount()` describes logical tiles, while `image()->arrayLayers()` remains one. Atlas bounds must fit signed 32-bit shader coordinates and the native image limits.

Copy the exported shared pointers to retain a snapshot. `cleanup()` drains users and drops this utility's references, leaving copied image/sampler owners valid. Replacement similarly preserves copied old snapshots. Device/layout/set ownership remains the caller's responsibility; Device outlives the utility and every exported owner. Perform build/cleanup only while producers are stopped. Disabled noise belongs to the effect's policy; it need not instantiate this utility.

## Explicit native shader sampling

Include `draw/lib/blue_noise.glsl` or `.metal`. `bg2eBlueNoise2` accepts the 2D atlas, metadata, signed pixel coordinates, uint32 rendered-frame and sample keys. Metadata must describe a valid uploaded atlas with at least two source channels. Logical layers are stacked in Y, with exact texel fetch/read after tile-local wrapping. Linear filtering, sRGB conversion, mip sampling and whole-atlas Y-repeat alter the intended rank values.

GLSL accepts separate texture2D and sampler function arguments: `bg2eBlueNoise2(noiseImage, noiseSampler, metadata, pixel, frame, sampleIndex)`. The helper constructs sampler2D directly inside texelFetch; a constructed combined sampler must not be passed to this user function under Vulkan GLSL rules. Metal reads texture2d directly and needs no sampler slot for read(). The exported sampler is available for callers needing normalized nearest sampling with correctly computed atlas coordinates. Bind noise separately or reserve any table entries explicitly; the utility consumes no hidden image/sampler-table capacity.

For disabled/missing GPU noise, select `bg2eHashNoise2(pixel,frame,sample)` through a shader variant/path that does not bind or access a noise texture. Never pass an unbound sampler as a fallback. Current scalar GPU bindings suffice; no general image-array primitive is introduced by this atlas layout.

[Manual verification](../../../tests/utils/blue_noise/README.md) covers CPU/native sampling and retained-owner lifetime. Advanced effects and bake integration remain later milestone work.

RendererDeferred shares one built atlas across available advanced effects that lack
an injected owner. Individual noise objects still take precedence; per-effect
useBlueNoise selects atlas versus hash and blueNoiseActive reports actual state.
No atlas is allocated solely for unavailable effects. See [advanced lighting](../../architecture/draw_advanced_lighting.md).
