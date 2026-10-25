# Base API Reference

Class and structure catalog for `bg2e::base`. Headers live in
[`lib/include/bg2e/base`](../../../lib/include/bg2e/base).
Each documentation page uses the same filename as its public header.

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

## Additional public types

| Header | Types |
|--------|-------|
| `Color.hpp` | `Color::Type` (`TypeSRGB`, `TypeLinear`). |
| `Light.hpp` | `Light::LightType`. |
| `Joint.hpp` | `LinkTransformOrder`. |
| `Texture.hpp` | `Texture::TextureType`, `Filter`, `AddressMode`. |
| `MaterialAttributes.hpp` | `MaterialAttributes::IndirectTextureMode`. |
| `PlatformTools.hpp` | `Platform`. |
| `Log.hpp` | `Log::Level` and `bg2e_log_*` macros. |
| `CpuProfile.hpp` | `CpuProfileSample`, `CpuProfileSnapshot`, `CpuProfileRecordingStatus`, `CpuProfiler::Collection`, `ContextProvider`. |
| `GpuProfile.hpp` | `GpuProfileStatus`, `GpuProfileBackend`, `GpuProfileWorkload`, `GpuProfileGranularity`. |
| `all.hpp` | Umbrella header, excluding the profiling headers. |

Private implementation details such as `Timeout::TimerEntry` are not application
APIs. See the individual pages for ownership, default values, and error behavior.

## Plugin resource path

`PlatformTools::pluginPath()` supplies the default native module directory;
see [PlatformTools](PlatformTools.md) and [plugin discovery](../plugin/lifecycle.md).
