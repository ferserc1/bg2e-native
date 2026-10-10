# LightmapSettings and result types

**Headers:** `<bg2e/render/LightmapSettings.hpp>`, `<bg2e/render/LightmapBaker.hpp>`
**Namespace:** `bg2e::render`

`LightmapMode { RTAO, RTGI }` chooses visibility or normalized indirect illumination.
`LightmapPixelFormat { RGB8, RGB32F }` selects CPU readback, independently of GPU storage.

| `LightmapSettings` field | Default | Contract |
|---|---|---|
| `resolution` | `512` | Positive square side length; attachment limits are checked against the device. |
| `mode` | `RTGI` | Supported enum value. |
| `accumulationFrames` | `16` | Positive number of sample updates. |
| `samplesPerPixel` | `8` | Positive, at most `INT_MAX`; rays per covered texel per update. |
| `giBounces` | `2` | Positive in RTGI; unused in RTAO. |
| `maxRayDistance` | `50.0f` | Finite, positive RTGI range in metres. RTAO uses 0.1 m. |
| `giRayBias` | `0.0005f` | Finite, nonnegative and below `maxRayDistance` in RTGI; normal-directed origin offset. |
| `exposureEV` | `0.0f` | Finite, within `[-16,16]`. Applied only to RGB8 RTGI readback as `2^EV` before clamp/round. |
| `cpuFormat` | `RGB8` | Supported enum value. |
| `dilationPixels` | `4` | Integer in `[1,32]`; one-pixel GPU expansion passes. |

Invalid settings throw `std::invalid_argument` when creating a baker. Settings
are copied and immutable through the baker's public interface. Create a new
baker to change them. Atlas padding and dilation are separate parameters.

## LightmapPixels

| Field | Default | Meaning |
|---|---|---|
| `width`, `height` | `0` | Image dimensions. |
| `format` | `RGB8` | Selects the expected alternative of `rgb`. |
| `rgb` | Default variant | `std::variant<std::vector<uint8_t>, std::vector<float>>`; tightly packed row-major RGB, `width * height * 3` elements. |

```cpp
bg2e::render::LightmapSettings settings;
settings.cpuFormat = bg2e::render::LightmapPixelFormat::RGB32F;
// After the baker has submitted at least one completed sample:
auto pixels = baker->readPixels();
const auto& rgb = std::get<std::vector<float>>(pixels.rgb);
```

RTAO returns grayscale visibility replicated into RGB. RTGI returns a
componentwise multiplier relative to unoccluded environment irradiance; it can
exceed one. RGB32F preserves float RGB without exposure; RGB8 clamps and rounds
once. No gamma correction or tone mapping is performed by readback.
The GPU result is RGBA32F; alpha is internal coverage and is omitted on CPU.

A dedicated RGB illumination material channel is still future work. Existing
material AO is a selected scalar channel; current bake exports reuse its texture
property. See [material integration and limits](../../architecture/lightmaps/output_and_materials.md).

Sources: [settings](../../../lib/include/bg2e/render/LightmapSettings.hpp),
[validation/readback](../../../lib/src/bg2e/render/LightmapBaker.cpp),
[conversion](../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp).
