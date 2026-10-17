# Public effect surface inputs

Header: `<bg2e/draw/pipeline/RTAmbientOcclusion.hpp>`.
SurfaceInputs and SurfaceProfile belong to `bg2e::draw::pipeline`; GI, reflection,
temporal and denoise reuse this contract without owning a camera renderer.

| Profile | Normal | Position source | Coverage |
|---|---|---|---|
| CameraDepth | RGBA16 float, encoded RGB mapped from [0,1] to signed world normal | D16/D32 depth and inverseViewProjection | Cleared zero normal means uncovered |
| WorldPositionCoverage | RGBA16 float, signed world XYZ | RGBA32 float world XYZ | Separate R8 UNORM mask |

All borrowed images are sampled, nonzero, same-device 2D images with matching
surface extents. Camera inverse VP must be finite and invertible. Explicit
positions/normals are world-space, not UV coordinates disguised as positions.
validate(device) checks the profile; extent() obtains its surface dimensions.
Signal extent may differ under the consuming effect's declared quality policy.
Caller retains all surface/output images through submitted work completion.

```cpp
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>

bg2e::draw::pipeline::SurfaceInputs surface;
surface.profile = bg2e::draw::pipeline::SurfaceProfile::WorldPositionCoverage;
surface.normal = normalImage;           // Borrowed sampled RGBA16 signed XYZ.
surface.depthOrPosition = positionImage; // Borrowed sampled RGBA32 world XYZ.
surface.coverage = coverageImage;        // Borrowed sampled R8 UNORM.
surface.validate(device);
```

The snippet assumes already allocated compatible images and a live gpu::Device&.
It does not generate surfaces or submit GPU work. No draw bake context or exact
sample-average API is implied by this input profile.

Reflections additionally borrow a material image (G=roughness), camera position
and explicit display extent. TemporalInput adds TemporalView and external history
generation. DenoiseInput adds worldToView for camera linear-depth comparisons;
explicit profiles use world-position distance. Geometry guides alone cannot
isolate coincident/nearby UV charts: the filter has no chart-ID input.

See [AO](RTAmbientOcclusion.md), [GI](RTGlobalIllumination.md),
[reflections](RTReflections.md), [histories](TemporalAccumulator.md) and
[denoise](DenoiseFilter.md).
