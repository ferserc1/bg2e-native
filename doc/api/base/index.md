# Base API

`bg2e::base` contains CPU data descriptions and shared services: colors, images,
texture/material settings, projection and lighting data, articulated transforms,
platform paths, dynamic libraries, logging, timers, and profiling telemetry.

```cpp
#include <bg2e/base/all.hpp>
```

The umbrella header includes the data/service headers below except `CpuProfile.hpp`
and `GpuProfile.hpp`, which must be included explicitly. Base texture/material
objects describe resources; rendering consumers realize the corresponding GPU data.

## Module areas

| Types | Documentation | Purpose |
|-------|---------------|---------|
| `DynamicLibrary` | [DynamicLibrary](DynamicLibrary.md) | Runtime dynamic-library ownership and symbol lookup. |
| `Camera` | [Camera](Camera.md) | Projection description and cached matrix. |
| `Color` | [Color](Color.md) | RGBA values and color-space identifiers. |
| `Image` | [Image](Image.md) | Owned CPU byte/float pixel storage. |
| `Texture, ProceduralTextureGenerator` | [Texture](Texture.md) | Texture descriptions and procedural sources. |
| `MaterialAttributes` | [MaterialAttributes](MaterialAttributes.md) | Material parameters, textures, and revision tracking. |
| `Light, LightData` | [Light](Light.md) | Illumination settings and transport data. |
| `Joint, LinkJoint` | [Joint](Joint.md) | Serializable articulated transforms. |
| `PlatformTools` | [PlatformTools](PlatformTools.md) | Platform and application resource paths. |
| `Log` | [Log](Log.md) | Stream-based diagnostic output. |
| `Timeout` | [Timeout](Timeout.md) | Polled delayed and repeating callbacks. |
| `CpuProfiler, CpuProfileScope, CpuProfileCollection` | [CpuProfile](CpuProfile.md) | Timing collection, scopes, recording, and CPU transport. |
| `GpuProfileInterval, GpuProfileSample, GpuProfileQueueStatus` | [GpuProfile](GpuProfile.md) | Bounded asynchronous GPU telemetry. |

## Where to go next

- [Quick start](quick_start.md) — small ownership and service recipes.
- [API reference](reference.md) — public headers and auxiliary types.
- [Dynamic library tutorial](../../tutorials/plugins/dynamic_library.md) — build a library and load it from a windowed application.
