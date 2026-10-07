# Step 06: Implement standalone batch control

## Scope

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, samples per pixel, GI bounces, RTGI maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Acceptance and compile gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `StandaloneBakeBatch` to coordinate one assembled scene, one `StandaloneBakerContext::updateScene`, one baker per target and exactly N synchronous update calls per baker. Each successful standalone submission advances the engine frame counter before CPU readback. The batch does not call `updateScene` inside its target loop.
- Reuse the public `geo::UvAtlasValidator` promoted in phase 2; do not create a second validator. Do not use `texCoord1` field existence as a test because importers copy UV1 when UV2 is absent. When regeneration is disabled, skip invalid targets with one warning each. When enabled, apply `geo::GenerateUv2AtlasModifier` to each eligible CPU mesh before GPU load and the single scene/TLAS update, then validate the generated UV2. Report baked and skipped counts.
- Pass square resolution, mode, samples per pixel, GI bounces and RTGI maximum distance to `LightmapSettings`. Progress callback reports `targetIndex`, `targetCount`, `completedFrames`, `accumulationFrames`; callback cancellation stops before the next submit and cleans owned resources.
- Validate scene inputs before the first TLAS update. A batch error must not leave half-written final files; defer writer commit to completed targets according to the output policy.

## Review example

With two valid target nodes and 16 frames, the batch builds the TLAS once and submits 32 target updates.

## Handoff

After this step, complete [prestep_07_application_integration.md](prestep_07_application_integration.md) for the next step (Create apps/lightmap_generator).
