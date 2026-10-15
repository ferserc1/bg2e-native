# M08-01 — Public standalone bake context and scene generation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M04/M05/M07 reviews.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakerContext.hpp`
- `lib/src/bg2e/render/StandaloneBakerContext.cpp`
- `lib/include/bg2e/bake/StandaloneBakerContext.hpp`
- `lib/include/bg2e/bake/StandaloneLightmapBaker.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakerContext.hpp](../../../../lib/include/bg2e/render/StandaloneBakerContext.hpp), [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../../../lib/src/bg2e/render/StandaloneBakerContext.cpp).

## Implementation work

1. Implement standalone Created/Initialized/SceneReady/Cleaned lifecycle and explicit scene preparation using a windowless draw engine/device, independent of a camera RendererDeferred.
2. Capture environment/light/material/geometry data and prepare isolated TLAS/resources using public draw/bake building blocks.
3. Bind baker instances to scene generation, reject stale/duplicate/invalid updates and define synchronous submission/completion for one accumulation sample.
4. Preserve target validation, root ownership, cleanup and producer coordination. No-RT hardware reports unsupported required baking instead of silently outputting unbaked maps.

## Verification and exit conditions

- A prepared context can bake multiple targets and invalidates prior bakers after a new scene generation.
- No main camera/window/UI or hidden production renderer is required to create resources; exceptions/cleanup do not leave active work.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-02/M08-04 use these types. M09 deletes the legacy standalone implementations only after equivalence is evidenced.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-08 standalone environment resources

Reuse the public EnvironmentGenerator/tool selections and validated snapshots from [environment-tools.md](../milestone_01_deferred_renderer/environment-tools.md). Load/generate before active recording, retain shared images/samplers through submissions and release all owners before Engine teardown. Empty source intentionally selects default sky; failed HDR input throws and preserves a previous published snapshot. Do not rebuild these algorithms privately in standalone bake.

## M01-13 reconstruction and settings

Read [upscaling.md](../milestone_01_deferred_renderer/upscaling.md). Reuse live resolution/upscaler settings identities where output reconstruction is exposed, with explicit drained lifecycle/reset boundaries in standalone contexts. SDK FSR temporal state is not a bake accumulation context or engine TemporalAccumulator.

## M01-16 reviewed baseline

M01 draw::Engine is still window-only, so this step requires the actual M05 generic/offscreen context, not an existing nonexistent headless Engine overload. Use public environment snapshots/collections and M03-selected surface/material addressing. Do not instantiate RendererDeferred to access private targets or reinterpret FSR context state as bake accumulation. See [M01 review](../milestone_01_deferred_renderer/review.md).
