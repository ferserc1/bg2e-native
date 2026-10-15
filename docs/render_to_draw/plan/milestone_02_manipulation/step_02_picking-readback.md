# M02-02 — Picking graphics and minimally sufficient GPU readback

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02-01 and M01 primitive/binding APIs.

## Scope and deliverable

- `lib/src/bg2e/manipulation/SelectionManager.cpp`
- `lib/include/bg2e/gpu/Image.hpp`
- `lib/include/bg2e/gpu/CommandBuffer.hpp`
- `lib/src/bg2e/gpu/metal/Image.cpp`
- `shaders/src/draw/picking/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/manipulation/SelectionManager.cpp](../../../../lib/src/bg2e/manipulation/SelectionManager.cpp), [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp), [lib/include/bg2e/gpu/CommandBuffer.hpp](../../../../lib/include/bg2e/gpu/CommandBuffer.hpp), [lib/src/bg2e/gpu/metal/Image.cpp](../../../../lib/src/bg2e/gpu/metal/Image.cpp).

## Implementation work

1. Reimplement the identifier/depth pick pass with draw geometry and explicit shader bindings. Preserve encoded RGBA8 baseline, pick kinds, stable per-snapshot ID mapping and unjittered view matrices.
2. Start from gpu::Image::readPixels (completed, tightly packed raw color mip/layer bytes on both backends) and readPixelsRGBA8 (R8G8B8A8_UNORM only), with explicit TransferSrc/initialized layout. Reuse full-image RGBA8 baseline unless a measured requirement justifies subregion/typed depth additions; current readPixels rejects depth. Native staging remains in GPU.
3. Define logical-window/drawable-pixel/DPI/viewport/Y conversion and synchronously complete the pick before mapping its result. Async readback is not required for parity.
4. Preserve click versus drag, additive/empty pick behavior and type/transform-gizmo priority as supported by current implementation. Do not promise unsupported node picking without implementing it.

## Verification and exit conditions

- Pick exact submeshes at multiple scales/DPI values and viewport offsets; blank/removed objects produce empty rather than stale selections.
- Query shadows, FSR/default scale and ordinary no-RT rendering remain usable during picks.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M02-06 viewpoint measurements reuse ID/depth primitives; M06/M07 selection widgets use the resulting stable map/lifetime contract.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-09 draw identity handoff

[bindings-visitors.md](../milestone_01_deferred_renderer/bindings-visitors.md) defines one-based frame object IDs (zero background), stable nodeId/submesh identity and canonical queue items distinct from transparent sort order. Retain the matching per-submission ID snapshot with readback; do not resolve an old GPU ID against a newly extracted queue. Visibility/world transforms come from the shared draw extractor, not legacy production visitors. Object slots retain GPU resources; keep an explicit CPU lookup for delayed selection callbacks.

## M01-10 raster surfaces

The actual [raster graph](../milestone_01_deferred_renderer/raster-layers.md) keeps six G-buffer channels, working depth and preserved opaque depth per acquired slot; no integer ID attachment/readback is implemented yet. Final debug depth is the last processed transparent/opaque layer and is not a picking surface. Add explicit ID/depth selection semantics and per-submission canonical objectId mappings here. Exact GPU copies support D16/D32 depth only; packed depth readback remains separate work.

## M01-16 reviewed baseline

Canonical RenderQueue IDs are one-based 24-bit per snapshot; SelectableComponent IDs are separately globally allocated. Define the mapping explicitly, retain nodeId/submeshIndex and weak scene ownership, and reject deleted/rebound snapshots. Do not use dense query instance order as an object ID. See [M01 review](../milestone_01_deferred_renderer/review.md).

## M02-01 actual handoff

M02-01 provides ToolExecutionMode, SelectionItem mode/drawMesh/selectionIdentifier and typed lockMesh/lockDrawMesh access, selectedDrawMesh and refreshSelection. Replace the explicit draw pick guards; keep CPU selection ready before GPU init. Map retained queue IDs into those weak references and reject node/component/resource/ID replacement. Do not confuse global Selectable IDs with frame queue IDs.
