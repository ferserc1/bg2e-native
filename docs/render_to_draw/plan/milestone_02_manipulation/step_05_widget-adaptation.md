# M02-05 — Material/texture/selection widgets by complexity

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M02-02–M02-04 and M01 reflected settings.

## Scope and deliverable

- `lib/include/bg2e/ui/MaterialEditor.hpp`
- `lib/include/bg2e/ui/TextureWidgets.hpp`
- `lib/src/bg2e/ui/MaterialEditor.cpp`
- `lib/src/bg2e/ui/TextureWidgets.cpp`
- `lib/include/bg2e/ui/ComponentInspector.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/ui/MaterialEditor.hpp](../../../../lib/include/bg2e/ui/MaterialEditor.hpp), [lib/include/bg2e/ui/TextureWidgets.hpp](../../../../lib/include/bg2e/ui/TextureWidgets.hpp), [lib/src/bg2e/ui/MaterialEditor.cpp](../../../../lib/src/bg2e/ui/MaterialEditor.cpp), [lib/src/bg2e/ui/TextureWidgets.cpp](../../../../lib/src/bg2e/ui/TextureWidgets.cpp), [lib/include/bg2e/ui/ComponentInspector.hpp](../../../../lib/include/bg2e/ui/ComponentInspector.hpp).

## Implementation work

1. Inventory widgets required by the manipulation example. Use simple draw overloads for material/texture controls when practical; introduce a separately named widget for structurally incompatible workflows.
2. Register preview textures through the private UI backend implementation using GPU objects; public headers expose no ImGui/native Vulkan types.
3. Route changed material values through their accessors/modifier and safe scene-update path. Keep the settings window from M01 dynamic rather than add fixed effect sections.
4. Document a widget-by-widget decision table for M06/M07, including scene-tree/property/UV/bake widgets not migrated here.

## Verification and exit conditions

- Material textures and previews update after safe reload/resize without stale registrations; the example needs no production Engine.
- Production widgets still operate and the decision table identifies each deferred widget owner/milestone.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06/M07 follow the actual overload/new-widget choices. M04 supplies bake UV previews; do not port them into draw here.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
