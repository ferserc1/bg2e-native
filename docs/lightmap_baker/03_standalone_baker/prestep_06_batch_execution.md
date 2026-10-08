# Handoff for Implement standalone batch control

Next implementation step: [step-06_batch_execution.md](step-06_batch_execution.md). This handoff records step 05 implementation findings; project-lead compile and dry-run/fixture verification remain pending.

- Changed files and relevant API decisions: Added `db::LightmapOutputWriter` in `lib/include/bg2e/db/LightmapOutputWriter.hpp` and `lib/src/bg2e/db/LightmapOutputWriter.cpp`, exported through `bg2e/db/all.hpp`. Construct it with the output directory and `db::ImageFormat`; call `preflight(targets, writeModelCopies, protectedInputPaths)` once before baking, then call `write(pixels, targetIdentity)` for each completed target. Preflight returns the final image and optional `.bg2` paths. `writeModelCopies` must match the UV2-generation branch.
- Build command, platform and result: Not run by the agent; plan and repository instructions reserve compilation for the project lead.
- Runtime/fixture evidence: Not run. Project-lead verification should dry-run and exercise full-batch path planning, output collisions and input aliases, no-overwrite behavior, output confinement, RGB32F-to-RGB8 clamp/round, image-only output when UV2 generation is disabled, and `.bg2` plus AO assignment when enabled.
- Remaining limitations or regressions: The `.bg2` serializer copies referenced non-AO material textures as sidecars into the output directory. Preflight reserves those names, rejects conflicting sources/outputs, and the writer stages copies before commit. Previous targets already committed remain intact if a later target fails; each individual target's image/model/sidecars are committed together with rollback on a write failure.
- Resources, ownership and synchronization cautions for the next agent: `Target` carries the stable `outputIdentity`, source `.bg2` path and attached node. Pass the assembled context JSON, model/prefab JSON and all non-target source `.bg2` resources in `protectedInputPaths`; target `.bg2` paths and loaded scene texture/environment resources are protected automatically. Call preflight for the entire target batch before the first bake or output write. The output `.bg2` is serialized from a Drawable clone, with one generated AO texture path shared by its submeshes, `aoUVSet = 1` and unit scale; loaded source materials are not modified. Images use canonical extensions; float RGB is clamped and rounded once to RGB8.
- Exact next action: Obtain project-lead compile and dry-run/fixture verification for standalone step 05; after it passes, implement `step-06_batch_execution.md`.

## Next-step instructions

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, samples per pixel, GI bounces, RTGI maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Next-step acceptance gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `StandaloneBakeBatch` to coordinate one assembled scene, one `StandaloneBakerContext::updateScene`, one baker per target and exactly N synchronous update calls per baker. Each successful standalone submission advances the engine frame counter before CPU readback. The batch does not call `updateScene` inside its target loop.
- Reuse the public `geo::UvAtlasValidator` promoted in phase 2; do not create a second validator. Do not use `texCoord1` field existence as a test because importers copy UV1 when UV2 is absent. When regeneration is disabled, skip invalid targets with one warning each. When enabled, apply `geo::GenerateUv2AtlasModifier` to each eligible CPU mesh before GPU load and the single scene/TLAS update, then validate the generated UV2. Report baked and skipped counts.
- Pass square resolution, mode, samples per pixel, GI bounces and RTGI maximum distance to `LightmapSettings`. Progress callback reports `targetIndex`, `targetCount`, `completedFrames`, `accumulationFrames`; callback cancellation stops before the next submit and cleans owned resources.
- Validate scene inputs before the first TLAS update. A batch error must not leave half-written final files; defer writer commit to completed targets according to the output policy.
