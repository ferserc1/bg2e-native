# M04-02 — Safe UV2 mesh rebuild and depthless UV surface primitives

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-01 and M03 reusable effect design.

## Scope and deliverable

- `lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp`
- `lib/src/bg2e/render/UvSurfacePass.cpp`
- `lib/include/bg2e/bake/UvSurfacePass.hpp`
- `lib/include/bg2e/gpu/Image.hpp`
- `shaders/src/bake/uv_surface/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp](../../../../lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp), [lib/src/bg2e/render/UvSurfacePass.cpp](../../../../lib/src/bg2e/render/UvSurfacePass.cpp), [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp).

## Implementation work

1. Keep the existing UV2 modifier/validator in geo and call it at a stopped-producer waitIdle boundary. Rebuild the draw Drawable GPU mesh/material/tracing representations after topology/UV changes.
2. Implement public bake UV surface recording: RGBA32F world position, RGBA16F signed normal, R32_UINT submesh/material identity and valid mask, with no depth.
3. Reuse existing depthless MRT, RGBA32F/RGBA16F/R32_UINT formats, typed clear validation, mip/face views and completed raw color readPixels. Decode raw integer/float bytes explicitly above staging; add GPU operations only for actually missing primitives. Depth readback and general 2D arrays remain absent; do not duplicate existing raw color transfer or assume automatic format conversion.
4. Define adapter/profile compatibility with public AO/GI/filter inputs selected by M03. Validate index ranges, UV coverage, target ownership and output extent.

## Verification and exit conditions

- UV2 alters only intended target geometry and clone-independent resources; invalid atlas/indices produce a clear error before submission.
- UV surfaces and integer masks have correct transforms/orientation on both available backends and match the selected effect profile.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04-03/M04-04 consume this profile; M06 editor reload and M08 standalone atlas paths must use the same waitIdle/rebuild/validation sequence.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-06 resource handoff

Use M01-06's standard `draw::Drawable::Mesh` (geo::MeshPNUUT) and `reload` after stopped-producer UV/geometry edits; setMesh handles submesh topology changes. Reload rebuilds raster/BLAS resources but preserves existing material/texture realizations and GPU-authored levels. Rebuild any external instances/bindings borrowing the replaced geometry. Independent clones must retain the implemented deep CPU/GPU/material/texture semantics. See [drawable-resources.md](../milestone_01_deferred_renderer/drawable-resources.md).

## M01-16 UV naming clarification

UV2 means the second channel: MeshPNUUT::texCoord1 and material UV selector 1. M01 accepts selectors 0/1; it does not have a third UV set at index 2. Reuse the geo atlas modifier for that second channel and explicitly refresh uploaded geometry/tracing and baked material semantics.

## M02 selection and edit lifetime

Use the stopped-boundary mesh update contract from the [M02 review](../milestone_02_manipulation/review.md). Replacing topology/CPU mesh identity via `setMesh()` can invalidate `SelectableComponent` IDs; changing UVs and calling same-topology `Drawable::reload()` does not necessarily do so. In either case, invalidate any in-flight bake/source generation, wait for users, rebuild draw raster/BLAS resources, and re-extract queues/bindings that retain the old GPU mesh. Do not use the pick ID as a bake target generation or assume `refreshSelection()` detects UV-only changes.

## M03-10 reviewed implementation requirement

Implement depthless UV surface outputs as RGBA32 world position, signed RGBA16 world normal and R8 coverage with matching extents. Generate explicit chart identity/isolation data for later atlas filtering. Do not fabricate a camera or reuse picking depth; GPU mesh reload creates a new tracing geometry identity.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
