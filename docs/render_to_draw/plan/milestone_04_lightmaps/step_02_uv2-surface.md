# M04-02 — Safe UV2 mesh rebuild and depthless UV surface primitives

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M04-01 and M03 reusable effect design.

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
3. Reuse generic MRT/subresource/binding operations; extend integer/storage/float typed readback or copy primitives in GPU only where actual kernels require it.
4. Define adapter/profile compatibility with public AO/GI/filter inputs selected by M03. Validate index ranges, UV coverage, target ownership and output extent.

## Verification and exit conditions

- UV2 alters only intended target geometry and clone-independent resources; invalid atlas/indices produce a clear error before submission.
- UV surfaces and integer masks have correct transforms/orientation on both available backends and match the selected effect profile.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04-03/M04-04 consume this profile; M06 editor reload and M08 standalone atlas paths must use the same waitIdle/rebuild/validation sequence.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
