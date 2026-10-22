# Draw model editor tool checks

`probe.hpp` exposes `model_edit::tests::runEditorToolsProbe(delegate, engine)`.
It replaces the open document: use a disposable draw_model_edit session after
its first renderer attachment and before frame acquisition. Add
`apps/model_edit/src` to the manual probe's include paths, include the header
in an application-owned check, and invoke at a stopped boundary. Do not call it
inside UI drawing or command recording. The engine and deployed gizmo/shader
assets must remain available throughout. This probe is not registered in CMake
and has not been compiled or executed by the implementing agent.

It asserts typed selection, partial material updates and stored/live agreement,
retention of authored image resources during sampler edits, independent cloned
CPU meshes/GPU textures, geometry edits affecting only the selected mesh, and
rejection of queued document/widget edits after document replacement while the
old nodes remain alive. It leaves a disposable replacement document open.

Project-lead interaction matrix:

1. Open BG2/OBJ/glTF/scene inputs, including multiple nodes referencing one glTF
   mesh. Edit one node's geometry; default editor resources must be independent.
   Select by viewport and list, use additive mode, clear selection on empty space,
   and inspect separate selection/highlight/gizmo identities.
2. Drag translate/rotate/scale handles, orbit/pan/zoom, and drag across a UI panel.
   Camera gestures must not select on release. A UI-owned press released over the
   scene must not pick; a captured gizmo must receive release over UI. Replace a
   document during pending input and confirm capture and camera drag state clear.
3. Rename nodes/submeshes; assign group/visibility to a multiple selection; edit
   node transforms; duplicate child and document-root selections; remove children.
   Check unique IDs, independent mesh/material/image resources and scene/BG2 saves.
   The document root cannot be removed. Runtime Selectable/Gizmo helpers must not
   prevent a valid one-model BG2 export or appear in exported documents.
4. Apply axis conversion, center, floor placement and cm-to-m actions. Verify
   normals/UVs, reload/BLAS refresh, overlays, history resets, dirty state and
   selection pruning after geometry replacement. Test available no-RT devices.
5. Edit scalar/color/alpha/PBR fields on one and multiple materials. Copy/paste a
   material with a GPU-only baked texture; edit the copy and compare the source.
   Apply JSON patches with omitted, malformed and invalid-path fields. Partial
   failures must retain diagnostics, dirty state and history invalidation. Saving
   unresolved pending texture changes must fail instead of exporting old pixels
   under newly requested settings.
6. Open all six texture sections: file picker/history, actual GPU preview, clear,
   UV/channel/scale/invert, color, filter, addressing, mip sampling and bias. Change
   sampler settings after GPU baking and verify native mip/pixel content survives.
   Choose ordinary AO after a baked map: mode must deliberately reset to AO.
   Baked AO/GI require an assigned map, use linear interpretation and UV set 1;
   a UV selection alone must never enable a baked interpretation.
7. Exercise sky visibility/normalized blur, shipped HDR presets and custom images.
   Force missing/invalid images; failed generation must preserve the prior GPU
   environment and current document. Add/edit/remove/select document lights up
   to the five-light editor limit, including type/color/intensity/shadows/pose and
   spot/source controls. Disable stage fill to inspect document lighting alone.
8. Save/restore environment presets and the explicit
   `draw_model_edit_environment.json` default. Compare world-space light poses
   beneath transformed document roots, floor height/visibility and fill toggle.
   Invalid root/components, missing images, unsupported child layouts, singular
   roots and excessive lights must fail before graph adoption. Production env.json
   must not be imported or overwritten automatically.
9. Queue edits, then Save/Open/close/Quit. Save must flush queued edits before
   snapshotting; dirty prompts must include pending changes. Cancel/discard/save,
   save failures and failed document loads must leave coherent document state.
   Check toolbar shortcuts, panel reopen, graphics save/restore/import and resize/
   minimize/DPI. Do not infer backend evidence from static checks.
10. Frame a small, large, off-center, transformed, single-point and empty model.
    Frame selection should use current camera FOV, and no-geometry input should
    leave the pose unchanged. Analyze best view with one and multiple submeshes;
    verify the winning candidate, score, coverage and top submesh pixel counts.
    Confirm floor/fill/camera fixtures are excluded, no-RT hardware works, a failed
    readback preserves camera pose, and document replacement before queued analysis
    cannot apply a stale candidate.
11. Compare the first window against production model_edit: maximized launch,
    full-width top toolbar/menu, docked Model Properties left, Environment Options
    right, status bar bottom, and the viewport between panels. Resize and change UI
    scale. Toggle both side panels through Window menu and toolbar buttons; check the
    menu checkmarks. Open UI and Graphics settings from Window menu and confirm the
    only separate utility windows. Check File/View/Window entries, camera reset/framing, and shortcuts.
12. Close while texture previews, gizmos, settings and edits are pending. Widget
    registrations and bindings must detach before renderer/stage/Engine teardown.
    Repeat with production model_edit to verify the shared input-router change.
13. Open Window → UV and Lightmap Baker. Compare UV1/UV2 diagnostics on a model
    with no UV2, generate a UV2 atlas and confirm the draw resource, selection and
    preview refresh only after the synchronized reload succeeds. Start AO and GI
    jobs at small and large resolutions; verify exact completed-sample progress,
    cancel after a submitted sample, and edit/replace the document during a job.
    Invalidated/stale targets must never publish or apply an image. Confirm completed
    results apply automatically to multi-submesh models, retain their map preview
    alongside UV1/UV2 previews, and inspect linear baked-AO/GI interpretation and
    UV2 metadata, then save/reload BG2 and scene documents and verify the generated
    GPU texture sidecar and live indirect contribution. Test failed UV generation,
    unsupported RT profiles, preview/window close and app teardown with pending work.

## Accepted editor corrections and focused checks

The project lead reports correct rendering with zero lights. Other scenario and
backend/package coverage remains unspecified. No probe was executed by the agent.

- Graphics Settings excludes scene/debug controls and retains Show unavailable
  settings. No Renderer Diagnostics menu or profiling controls appear in Graphics
  Settings. Export/restore must not persist diagnostic selections or environment/light values.
- Change a custom environment image back to Procedural sky. Edit stage/document
  lights through reflected translation/rotation/scale and light properties.
  Unbounded numeric fields support drag and Ctrl+click/double-click text entry;
  bounded rotations keep their angle controls.
- Only light gizmos are visible. Check the initial light at `(2, 2, 2)` and added
  light local ranges X/Z `[-3,3]`, Y `[0,3]`. Remove every light and save/restore
  the environment preset; no minimum count or automatic replacement is imposed.
- Positive-area UV2 overlaps warn and continue. Completed RGB8 maps use a temporary
  linear PNG, float maps retain dynamic range in standard Radiance HDR with RGBE quantization, and complete save/reload
  keeps final standard image resources after temporary paths are unavailable.
