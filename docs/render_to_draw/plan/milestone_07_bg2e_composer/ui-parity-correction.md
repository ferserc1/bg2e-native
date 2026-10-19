# Composer UI parity correction after M07 review delivery

The project lead rejected the inherited Model Edit layout/Environment Options
panel and requested the production Composer interaction model. This corrects the
M07-03 UI and the M07-07 review's parity description; it does not implement M07-08
or accept the pending review. No production app source or CMake is changed.

## Actual replacement

- Left: **Model Properties**, the selected drawable's submesh selection,
  names/groups/visibility, geometry operations, material clipboard/patches and
  DrawMaterialEditor. Multi-node selection renders material controls only.
- Right: **Scene**, split into independent tree and node-inspector child regions,
  each taking half/remaining space as in production SceneEditor. The inspector
  edits the selected node's name/enabled state; multiple nodes show the production
  multiple-selection notice rather than editing an arbitrary primary node.
- Environment Options, its draw function, presets, duplicate light/camera editors
  and menu/toolbar entry are removed. Environment/light/camera editing is through
  authored node components. Existing legacy environment service helpers are not
  exposed as a second panel. Inherited floor/fill fixtures start disabled; a
  document environment replaces the stage fallback when scene bindings refresh.
- The actual shared `ui::ComponentInspector` renders the production **Add Component**
  button/dropdown, registered/reflected type choices sorted by display name with
  duplicates excluded, and collapsible component headers with **Remove**.
  The previous app-local hard-coded component button list is gone. Node creation
  remains separately available through the toolbar's Create Node menu.
- Optional add/remove/draw-component callbacks extend ComponentInspector without
  exposing ImGui or changing default production behavior. Composer uses weak
  intent callbacks and existing document/member/generation-checked queues. Factory
  construction, draw resource loading and camera/light validation run at the safe
  boundary; ordinary add failure removes the candidate. CPU component properties
  are staged on independent exact-type clones. Drawable GPU resources are never
  cloned per UI frame; their editor is in Model Properties.
- Inspector property/enabled edits retain the selected authored node; stale
  component/submesh handles refresh at the queued edit boundary rather than
  deselecting the node after every field change.
- Tool preparation no longer invents an authored Transform on every node, so
  removing one through the inspector does not immediately recreate it. Transform
  gizmos require an authored Transform; created nodes still receive one.

## Validation handoff

Agent inspection covers declarations/callbacks, shared production default paths,
weak ownership/queued mutation, scope balance, source style/GPL, local links and
diff whitespace. No compilation, app, shader, test or GPU execution.

Lead checks on available Vulkan/Metal:

1. Left drawable/material and right Scene layout, resize/scroll, hide/reopen menu
   checkmarks and toolbar buttons. No Environment Options entry/panel remains.
2. Tree selection of mesh/group/camera/light/environment, no/multiple selection,
   and submesh/material selection in the left panel without a table-scope crash.
3. Add Component dropdown matches registered production options, excludes existing
   types, and supports Chain. Remove/re-add Transform without automatic recreation.
   Duplicate/remove node toolbar actions still work.
4. Edit camera/light/environment through components; changing HDR affects the
   document environment rather than a hidden fixture. Last-camera/ninth-light
   rejection, failed factory/resource load and stale queued selection preserve a
   usable document. Check dirty/history/bake invalidation and save/reload.
5. Regression-check production ComponentInspector/NodeEditor default add/remove/
   reflected resource behavior and draw_model_edit's unchanged environment panel.

Known extension contract remains: custom reflected components provide independent
exact clones and draw resource hooks. Native runtime results remain lead-owned.

## Subsequent Open Scene report

The lead reports Open appears to do nothing, without specifying whether the
chooser opens. Named NFD guards now keep native initialization alive through
open/save dialogs. Composer displays chooser/load exceptions visibly and preserves
operation errors alongside dirty status. This corrects two verified source defects,
not a claimed runtime reproduction of the exact report. Check chooser/cancel,
valid scene replacement and invalid/missing-resource diagnostics before acceptance;
existing camera/schema/resource validation remains active.
