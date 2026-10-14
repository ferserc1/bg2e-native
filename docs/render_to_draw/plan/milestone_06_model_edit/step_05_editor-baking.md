# M06-05 — Model UV2 preview and integrated lightmap workflow

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M06-02/M06-03 and accepted M04 APIs.

## Scope and deliverable

- `apps/model_edit/src/ModelLightmapWindow.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`
- `lib/include/bg2e/bake/UvMapPreviewRenderer.hpp`
- `apps/draw/model_edit/src/ModelLightmapWindow.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/model_edit/src/ModelLightmapWindow.cpp](../../../../apps/model_edit/src/ModelLightmapWindow.cpp).

## Implementation work

1. Port the lightmap window/jobs to bake settings/context/pixels and configurable public draw effects, preserving target validation/progress/reset/cancel behavior.
2. Perform UV2 generation and Drawable rebuild at the coordinated waitIdle boundary, and refresh tracing state, UV previews and selected submesh references.
3. Apply completed maps to both CPU definitions and live material resources with correct RGB multiplier/UV set metadata, and persist/export through complete resource-save paths.
4. Coordinate async loading, scene locking and window/job teardown so no context borrows a replaced renderer/environment or deleted node.

## Verification and exit conditions

- Generate/preview/bake/apply/save/reload lightmaps on a model and verify live indirect toggling/baked contribution.
- Cancel/reopen/load/delete/renderer replacement during bake follows the declared safety policy and leaves UI/document state correct.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M07 scene-wide jobs and M08 standalone batch adapt these proven target/publication/export semantics; they must not copy window scheduling into CLI.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
