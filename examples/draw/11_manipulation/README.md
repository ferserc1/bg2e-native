# Manipulation example

Existing `draw_manipulation` product with shared renderer settings, selection and gizmos.

## Advanced effect controls

The shared Graphics settings window exposes renderer IBL/AO/GI selection,
independent reflection controls, query shadows, temporal accumulation, spatial
denoise, resolution, optional FSR and optional SMAA. Enable the effect as well as
selecting its indirect-lighting mode. Requested values persist even on unsupported
hardware; inspect read-only availability/status and graph selection separately.
Both FSR and SMAA start disabled, preserving ordinary resampling. SMAA processes
final scene color before overlays/UI and also works without RT.

Pause camera/light animation to inspect temporal convergence. Scene/material and
gizmo edits reset SDK/engine history through the synchronized application boundary.
Use lighting debug modes for resolved ray visibility, diffuse GI, reflected
radiance and fractional reflection confidence. Disable final color processing/use
Raw HDR when examining numeric HDR signals. The last processed layer is shown.

[Integration and capability limits](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/integration.md)
· [Lead validation matrix](../../../tests/draw/advanced_integration/README.md).

### Opening graphics settings

The separate **Graphics settings** window opens beside the scene controls at
startup. If closed, use the **Graphics settings** button at the top of the scene
controls to reopen it. Expand **Renderer** to select **Indirect lighting = GI**,
then **Global illumination > Enabled**. Reflections have an independent
**Reflections > Enabled** switch. Read **Available** and **Status** in each effect
section. Save/restore preferences and JSON import/export use the same live catalog;
no preferences are restored automatically. Each example uses its own preference scope.

## Light editing and initial GI

This example now starts with renderer indirect lighting set to GI, with both
RTGlobalIllumination and RTReflections enabled. Unsupported devices still report
availability/status and retain the existing raster/IBL fallback. These defaults
apply only to draw_manipulation; FSR and SMAA remain disabled initially.

Expand **Manipulation > Lights > Light 1..4** to edit each light's type, color,
intensity, spot angles, shadow casting, source size, sample budget and reflection
participation, plus **Position XYZ (local)** and **Orientation XYZ (degrees)**.
Spot angle controls are available for spot lights. Omni lights ignore orientation;
spot/directional lights use the transformed local +Z direction. Transforms are
relative to the Lights parent, whose rotation is retained when animation pauses.
Editing through this section automatically pauses light animation. Resume with
Animate lights if desired; pause it to inspect temporal convergence.

Properties reuse live catalog descriptors and the same validated queued edit path
as Graphics settings. Transform targets are example-owned `lightPose/<nodeId>`
entries and also participate in JSON/preference save/restore. Transform edits
preserve translation/rotation/scale independently, stop animation and invalidate
histories at the synchronized update boundary. No imgui types or direct GPU/UI
scene mutation are introduced. Runtime validation remains with the project lead.
