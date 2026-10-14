# M01-14 — Dynamic graphics settings widget and minimal texture UI

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-12/M01-13.

## Scope and deliverable

- `lib/include/bg2e/ui/ReflectionWidget.hpp`
- `lib/include/bg2e/ui/RenderSettingsWindow.hpp`
- `lib/include/bg2e/ui/TextureWidgets.hpp`
- `lib/include/bg2e/ui/GraphicsSettingsWindow.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/ui/ReflectionWidget.hpp](../../../../lib/include/bg2e/ui/ReflectionWidget.hpp), [lib/include/bg2e/ui/RenderSettingsWindow.hpp](../../../../lib/include/bg2e/ui/RenderSettingsWindow.hpp), [lib/include/bg2e/ui/TextureWidgets.hpp](../../../../lib/include/bg2e/ui/TextureWidgets.hpp).

## Implementation work

1. Choose a new reflected GraphicsSettingsWindow for the substantially changed settings workflow; retain production RenderSettingsWindow until M09 instead of mixing incompatible option models.
2. Render the active catalog using existing UI wrappers/reflection editors, instance capability filtering and stable property handles. Rebuild sections on catalog-generation changes and show unsupported optional methods meaningfully.
3. Provide settings file export/import and programmatic persistence controls without becoming the source of configuration values. Apply GPU-changing settings at safe update boundaries.
4. Only adapt preview/texture widgets required by this example through simple engine overloads/native-free handles. Full MaterialEditor/picking widgets belong to M02/M06.

## Verification and exit conditions

- UI values agree with C++ accessors and imported files; switching renderer/upscaler changes available controls without retaining dead pointers.
- No ImGui/Vulkan handle types leak into new public widget interfaces. Production settings window remains operational.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02/M06/M07 evaluate each remaining widget separately; M03 adds reflected effect sections rather than rewrite the settings window.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
