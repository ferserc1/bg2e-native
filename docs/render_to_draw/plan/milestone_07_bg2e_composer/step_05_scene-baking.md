# M07-05 — Scene-wide bake jobs, previews and complete saves

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M07-02–M07-04 and accepted M04/M06 bake integration.

## Scope and deliverable

- `apps/bg2e_composer/src/SceneLightmapWindow.cpp`
- `apps/bg2e_composer/src/AppDelegate.cpp`
- `apps/draw/bg2e_composer/src/SceneLightmapWindow.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/src/SceneLightmapWindow.cpp](../../../../apps/bg2e_composer/src/SceneLightmapWindow.cpp), [apps/bg2e_composer/src/AppDelegate.cpp](../../../../apps/bg2e_composer/src/AppDelegate.cpp).

## Implementation work

1. Adapt multiple target jobs to bake contexts/settings/pixels, shared public effect objects and correct target/scene-generation validation.
2. Preserve UV2 safe rebuild, preview, progress/cancel/reset and application of RGB/UV/material metadata across selected or scene-wide targets.
3. Export/load resources through complete DB paths, including temporary output/publication and unsaved state. Do not make a live renderer/context outlive its environment/scene during async import.
4. Handle unavailable RT bake hardware explicitly while preserving scene editing and no-RT raster rendering.

## Verification and exit conditions

- Several targets bake/apply/save/reload correctly without swapping material/submesh identities or losing dilation/pixel semantics.
- Cancel/import/delete/renderer replacement invalidates jobs and borrowed resources according to the reviewed lifecycle.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08 standalone jobs preserve target/output identity and validation but use a windowless scheduler instead of this integrated UI path.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
