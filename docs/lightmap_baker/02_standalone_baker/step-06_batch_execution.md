# Step 06: Implement standalone batch control

## Scope

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, shadows, samples per pixel, GI bounces, maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Acceptance and compile gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `StandaloneBakeBatch` to coordinate one assembled scene, one `StandaloneBakerContext::updateScene`, one baker per target and exactly N synchronous update calls per baker. The batch does not call `updateScene` inside its target loop.
- Reuse the private usable-UV2 validator from integrated step 02. Phase 3 promotes it to `geo::UvAtlasValidator`; do not create a second validator. Do not use `texCoord1` field existence as a test because importers copy UV1 when UV2 is absent. Skip invalid targets with one warning each and continue; report the number baked and skipped.
- Pass square resolution, mode, shadow switch, samples per pixel, GI bounces and maximum distance to `LightmapSettings`. Progress callback reports `targetIndex`, `targetCount`, `completedFrames`, `accumulationFrames`; callback cancellation stops before the next submit and cleans owned resources.
- Validate scene inputs before the first TLAS update. A batch error must not leave half-written final files; defer writer commit to completed targets according to the output policy.

## Review example

With two valid target nodes and 16 frames, the batch builds the TLAS once and submits 32 target updates.

## Handoff

After this step, complete [prestep_07_application_integration.md](prestep_07_application_integration.md) for the next step (Create apps/lightmap_generator).
