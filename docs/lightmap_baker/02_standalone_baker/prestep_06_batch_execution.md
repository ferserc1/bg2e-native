# Handoff for Implement standalone batch control

Next implementation step: [step-06_batch_execution.md](step-06_batch_execution.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-06_batch_execution.md.

## Next-step instructions

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, samples per pixel, GI bounces, RTGI maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Next-step acceptance gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `StandaloneBakeBatch` to coordinate one assembled scene, one `StandaloneBakerContext::updateScene`, one baker per target and exactly N synchronous update calls per baker. Each successful standalone submission advances the engine frame counter before CPU readback. The batch does not call `updateScene` inside its target loop.
- Reuse the private usable-UV2 validator from integrated step 02. Phase 3 promotes it to `geo::UvAtlasValidator`; do not create a second validator. Do not use `texCoord1` field existence as a test because importers copy UV1 when UV2 is absent. Skip invalid targets with one warning each and continue; report the number baked and skipped.
- Pass square resolution, mode, samples per pixel, GI bounces and RTGI maximum distance to `LightmapSettings`. Progress callback reports `targetIndex`, `targetCount`, `completedFrames`, `accumulationFrames`; callback cancellation stops before the next submit and cleans owned resources.
- Validate scene inputs before the first TLAS update. A batch error must not leave half-written final files; defer writer commit to completed targets according to the output policy.
