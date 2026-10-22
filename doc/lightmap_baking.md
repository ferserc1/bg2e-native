# Lightmap baking

`bg2e::bake` provides integrated and standalone UV2 baking over `bg2e::gpu`.
Include `<bg2e/bake/all.hpp>`. The production scene/material path is draw; there
is no separate native renderer baking API.

## Choose a workflow

- [Integrated scene baking](tutorials/draw/12_lightmaps.md): interactive progress,
  cancel/restart, preview and material adoption in a running draw application.
- [Standalone C++ baking](tutorials/draw/standalone_lightmaps.md): camera-less
  scene assembly, one live target/context, serial batches and protected publication.
- [Lightmap generator CLI](apps/draw_lightmap_generator.md): model/prefab input,
  typed settings, preflight, output manifests, interruption and overwrite rules.

## Signals and samples

UV2 is independent of texture UV1. Chart isolation and rasterized coverage prevent
unrelated charts sharing accumulated values. Each completed raw update contains
the configured samplesPerPixel estimate; equal frozen ray counts allow equal
update weighting. AO is visibility replicated across RGB. GI output is a linear
indirect multiplier referenced to unoccluded environment irradiance. Uncovered
pixels are neutral white; invalid covered values fail validation. Lightmaps do not
contain receiver albedo, alpha or display gamma.

RGB32F preserves finite nonnegative HDR. RGB8 applies the documented exposure and
clamp/round conversion. GPU floating image precision and CPU output format are
separate. Counts are exact; floating sums have normal float32 rounding. Cancelled
or uncompleted submissions do not publish successful results.

## Ownership and output

Device outlives context, passes, preview and result images. Injected mutable bake
passes belong to one bake lifecycle and never share mutable display slots.
Complete a frame slot before reuse, retain all borrowed submission owners and
stop/drain before teardown. Restart after settings/geometry generation changes.
Finished results carry UV2/indirect semantics and source/target generations.
Material adoption uses the explicit RGB indirect map contract in the AO slot;
a separate scalar AO texture cannot occupy that same slot simultaneously.

Use the complete output writer for BG2 model/material resources and standard PNG/HDR images and
manifest publication. A saved GPU sampler alone is not a materialized image.
Camera PNG export is a separate [offscreen workflow](apps/draw_offscreen_application.md).

## Reference

- [Bake API and live settings](api/bake/index.md)
- [Contexts](api/bake/BakerContext.md), [integrated context](api/bake/IntegratedBakerContext.md)
- [Accumulation and output](api/bake/LightmapOutput.md)
- [Standalone batch](api/bake/StandaloneBakeBatch.md)
- [Output writer](api/bake/LightmapOutputWriter.md)
- [Baked material sampling](api/draw/BakedIndirectMaps.md)
- [UV atlas and diagnostic concepts](architecture/lightmaps/uv_atlas.md)
