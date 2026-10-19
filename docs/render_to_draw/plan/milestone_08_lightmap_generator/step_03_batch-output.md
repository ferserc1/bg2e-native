# M08-03 — Bake batch/output writer and protected complete exports

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01/M08-02 and M04 pixel contracts.

Keep complete output/preflight identities separate from M02 selection/pick IDs.
MaterialModifier and draw UI edit paths are nontransactional and do not own batch
publication or atomic saves; standalone jobs must snapshot source generation,
revalidate before writing and preserve protected-input semantics after failures.
M02's clone implementation copies live resources but is not a resource exporter.
See [M02 review](../milestone_02_manipulation/review.md).

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakeBatch.hpp`
- `lib/src/bg2e/render/StandaloneBakeBatch.cpp`
- `lib/include/bg2e/db/LightmapOutputWriter.hpp`
- `lib/src/bg2e/db/LightmapOutputWriter.cpp`
- `lib/include/bg2e/bake/StandaloneBakeBatch.hpp`
- `lib/include/bg2e/bake/LightmapOutputWriter.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakeBatch.hpp](../../../../lib/include/bg2e/render/StandaloneBakeBatch.hpp), [lib/src/bg2e/render/StandaloneBakeBatch.cpp](../../../../lib/src/bg2e/render/StandaloneBakeBatch.cpp), [lib/include/bg2e/db/LightmapOutputWriter.hpp](../../../../lib/include/bg2e/db/LightmapOutputWriter.hpp), [lib/src/bg2e/db/LightmapOutputWriter.cpp](../../../../lib/src/bg2e/db/LightmapOutputWriter.cpp).

## Implementation work

The [M07 review](../milestone_07_bg2e_composer/review.md) distinguishes three
contracts: complete single-model/scene DB publication, Composer per-item selected
export, and temporary bake-texture material adoption. None supplies this step's
whole-batch preflight/protected-input/overwrite guarantees. Do not copy editor
partial-material failure handling as a batch transaction. Retain typed RGB32F HDR
without imposing Composer's interactive half-float display limit on file output.

1. Place specialized bake batch execution and output/preflight/model-copy orchestration in bake, including LightmapOutputWriter; keep generic image/scene file functions in DB.
2. Preserve complete-batch output planning, duplicate identities/path collision checks, protected input assets/sidecars, explicit overwrite policy and atomic replacement semantics.
3. Write LightmapPixels RGB8/RGB32F maps with format/conversion/dilation metadata and optional model copies that reference the generated resource correctly.
4. Reuse complete high-level save/readback paths; preserve production DB writer entry points as thin legacy-compatible overloads during coexistence rather than break the old CLI early.

## Verification and exit conditions

- Preflight catches target/output/input collisions before baking/export, and overwrite permission is honored precisely.
- Byte/float outputs and model copies reload with correct resource/UV/multiplier semantics; no original source asset is overwritten accidentally.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-04 consumes this orchestration. M09 removes production writer wrappers and all render::LightmapPixels dependencies; bake remains the owner.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M05 single-image publication is not a bake batch transaction

M05's example writes one known RGBA8 camera PNG to a PNG-suffixed temporary path,
verifies decode dimensions/bytes, then publishes with a no-clobber same-directory
hard link. Failure removes the temporary image. This does not implement complete
batch preflight, overwrite/replace policy, multi-file rollback, model-copy
publication, source-sidecar protection or resource-reference rewriting. M08 must
implement and validate those batch guarantees around typed LightmapPixels and
complete M06/M07 resource saves. Keep lightmap RGB8 exposure and RGB32F HDR
encoding/orientation separate from the camera PNG policy.

## M01-16 reviewed baseline

M01 raw/reference JSON IO and raw completed color readback are building blocks, not complete batch/save semantics. Consume real M04 LightmapPixels and M06/M07 full resource-save contracts before implementing specialized bake writer/preflight/atomic publication. Preserve explicit byte/float color/orientation metadata and input protection. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M03-10 reviewed implementation requirement

Reuse M04 chart-isolated raw-signal averaging and declared validity/HDR normalization, not TemporalAccumulator frameCount or denoised histories. Retain immutable snapshots and output owners until submitted work completes on cancel/error. Preflight required tracing/capacity before model/map output; preserve protected-input/overwrite guarantees.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-04 actual output handoff

Reuse public UvTemporalAccumulator/LightmapCompositionPass with completed history and fresh per-record outputs. Standalone scheduling must commit only completed current jobs, validate all requested covered counts/numerics, and compose/dilate before exporting LightmapPixels. Keep exact integer counts/raw float sums separate from denoise, preserve source/target/geometry identity and reject current half-float GI saturation at 65504. GPU/CPU output shares nearest-chart padding and linear top-row-v=0 semantics; float HDR has no exposure/clamp, byte GI alone applies exposure. Final validation/readback is synchronous, not a new async staging interface.

See [output implementation](../milestone_04_lightmaps/accumulation-output.md).


## M04-07 reviewed implementation requirement

Export the completed typed RGB pixels and real image files before publishing model/material copies. Preserve indirectTextureMode strings (ao/bakedAO/bakedGI), UV2 and linear color on reload. A retained sampled GPU image and copied sampler settings are not an exported resource. Keep RGB32F HDR output distinct from the interactive RGBA16F range limit; apply exposure/clamp only to the explicit RGB8 GI conversion. Implement actual batch/writer types here: current bake namespace only forward-declares Batch/SceneAssembler and supplies no new writer.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).


## M06-02 implemented resource publication baseline

Reuse the [complete draw IO contract](../milestone_06_model_edit/resource-io.md):
explicit engine-aware scene/model save snapshots native GPU color mips/layers and
publishes unique resources before replacing the main file. `.bg2tex` preserves
HDR and requires draw texture decoding; raw/schema/reference IO still exports no
resources. Custom schema/reference/draw hooks preserve component fields; custom
external-resource types require an explicit export contract. Do not route this
through production image loaders. Format-specific batch image outputs/conversions
remain this consumer's responsibility; native archive support does not implement
the pending batch writer or scheduler. Preserve previous input generations and
mark document/batch success only after output publication.

The [M06 review](../milestone_06_model_edit/review.md) confirms complete native
draw saves materialize GPU-only texture mip/layer bytes into `.bg2tex` resources
before atomically publishing the main scene/model file. That is resource snapshot
IO, not the M08 output contract: implement typed RGB8/RGB32F map encoding,
complete-batch collision/preflight, overwrite policy, protected source/sidecar
semantics and coordinated optional model-copy publication here. Do not treat
sampler metadata or the model-editor Apply action as an output writer.
