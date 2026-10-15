# M01-14 — Dynamic graphics settings widget and minimal texture UI

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-12/M01-13.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M02/M06/M07 evaluate each remaining widget separately; M03 adds reflected effect sections rather than rewrite the settings window.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-12 actual facade and queue

Consume [reflected-settings.md](reflected-settings.md). Enumerate catalog.properties(), read live values, retain generation-bearing SettingHandles, and use queueSet/queueImport plus completion diagnostics. Refresh after replacement or availability/type changes; includeUnavailable=true exposes capability diagnostics. Never invoke stored PropertyInfo setters on delayed raw objects. The facade connects weak lifecycle callbacks before initial renderer binding and can be composed explicitly when callbacks are already owned. Use SettingsCodec file APIs; the legacy JsonNode writer loses numeric precision/escaping. Persistence is opt-in; an explicit document bypasses persisted defaults. No UI initialization is needed by the codec.

## M01-13 actual upscaling settings

Read [upscaling.md](upscaling.md). Enumerate live `resolution` (enabled/scale) and `upscaler/<identifier>` targets automatically supplied by RenderSettingsPreferences; do not add a closed upscaler enum or a copied quality model. Scale is the authoritative [0.25,1] ratio, independent of SDK availability. FsrUpscaler exposes SDK/backend/hardware/status separately. Queue edits before acquisition; extent/needsRebuild changes detach, resize and rebind catalog generations. Implementation replacement uses queued renderer configuration replacement. Preserve FinalLayer live color settings.

## M01-14 implementation handoff

See [settings-ui.md](settings-ui.md) for GraphicsSettingsWindow, transient ReflectionWidget::editValue, facade queueRestore/hasPersistence, strict file IO, weak completions and lead checks. ResourcePicker supplies the HDR file intent required by the reference example; it uses no legacy TextureWidgets preview. Full material/preview adaptation remains M02/M06.
