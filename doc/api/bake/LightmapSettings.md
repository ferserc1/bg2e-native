# LightmapSettings

**Header:** `<bg2e/bake/LightmapSettings.hpp>` · **Namespace:** `bg2e::bake`

This is the actual live bake settings object. Share it with
LightmapBakerConfiguration and edit it only at a stopped job boundary. The baker
freezes a validated copy when created/reset; changing the live settings does not
modify an in-flight job. `settingsInstance()` returns this instance and
`settingsType()` describes it using `bg2e.bake.lightmapSettings.v1`. The example
uses ReflectionWidget against this object while no job is running; there is no
parallel editable preferences model.

Each property has a getter named below and a corresponding `setX` setter.

| Getter | Library default | Accepted values |
|---|---|---|
| resolution | 512 | 1–16384, also limited by the device |
| mode | LightmapMode::RTGI | RTAO or RTGI |
| accumulationFrames | 16 | 1–1048576 completed updates |
| samplesPerPixel | 8 | 1–64 rays per update |
| giBounces | 2 | 1–8 |
| aoRadius | 0.1 | finite, 0.000001–1000000000 |
| maxRayDistance | 50 | finite, 0.000001–1000000000 |
| rayBias | 0.0005 | finite, 0.000001–1000000000 |
| exposureEV | 0 | finite, -32–32; CPU byte GI conversion only |
| cpuFormat | LightmapPixelFormat::RGB8 | RGB8 or RGB32F |
| dilationPixels | 4 | 0–16384, also no greater than resolution |

Setters reject invalid enum/range/nonfinite values. Call `validate()` after all
paired edits: selected AO radius/GI distance must exceed twice the bias, and
padding must not exceed resolution. Device/profile checks belong to preparation,
not this CPU-only validation. The shipped example deliberately overrides
resolution to 128, rays to 4 and CPU format to RGB32F; other defaults remain.

One accumulation update is not one ray and not one presentation frame. The raw
sample key is the number of completed updates. Final result publication requires
all requested valid samples; an unsupported effect cannot produce a successful
neutral map. [Output semantics](LightmapOutput.md) explains RGB8 exposure,
float precision, normalization and padding.

Source: [header](../../../lib/include/bg2e/bake/LightmapSettings.hpp),
[implementation/metadata](../../../lib/src/bg2e/bake/LightmapSettings.cpp).
