# M04-04 — Bake accumulation, composition, dilation and typed pixels

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-02/M04-03.

## Scope and deliverable

- `lib/src/bg2e/render/UvTemporalAccumulator.cpp`
- `lib/src/bg2e/render/LightmapCompositionPass.cpp`
- `lib/include/bg2e/bake/UvTemporalAccumulator.hpp`
- `lib/include/bg2e/bake/LightmapCompositionPass.hpp`
- `shaders/src/bake/accumulation/`
- `shaders/src/bake/dilation/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/UvTemporalAccumulator.cpp](../../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp), [lib/src/bg2e/render/LightmapCompositionPass.cpp](../../../../lib/src/bg2e/render/LightmapCompositionPass.cpp).

## Implementation work

1. Implement UV-space progressive sample accumulation with valid masks, distinct AO/HDR GI signal semantics and public configurable kernels/resources.
2. Compose the normalized RGB indirect-light multiplier with neutral white invalid/unbaked behavior and current exposure/settings semantics; preserve material application metadata.
3. Apply edge dilation consistently to sampled output and CPU-exported data. Provide RGB8/RGB32F LightmapPixels through correctly completed GPU readback.
4. Add full output validation and progress/reset accessors; avoid forcing a window or viewport renderer for a result record.

## Verification and exit conditions

- Known AO/GI fixtures produce sensible accumulated multipliers; border dilation is visible and does not change valid interior semantics.
- Float output preserves HDR range and byte output matches its declared conversion; incomplete/stale results cannot be read as current.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05 readback exports and M08 writer/batch code reuse actual pixel/conversion semantics. Editors must apply output with the correct AO/lightmap and UV2 flags.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
