# M07-06 — Composer product validation record

## Selection-crash correction after delivery

Subsequent lead outcome: explicitly checked and accepted with the instruction to
continue to M07-07. Backend/package coverage beyond that statement is unspecified.
The pending-validation text below records the original correction handoff.

The lead reports a crash selecting an object, with `g.CurrentTable == nullptr`
in ImGui::TableNextRow. Static inspection identifies a concrete invalid call chain:
draw Composer's SceneEditor submesh loop calls SelectableList::item without opening
a table. This is an implementation bug in the delivered inspector, not a missing
future milestone feature or evidence of an ImGui/native-rendering defect.

The inspector now opens/closes the list. Shared SelectableList::beginList returns
BeginTable's bool instead of discarding it; all five repository calling paths
conditionally submit rows/endList only on success (Composer, both model editors,
SubmeshSelector and GPUSelectionDialog). This also fixes the latent skipped-table
case. No ImGui/vendor source, CMake or rendering code changed. Public UI references
describe the new contract. See [regression checks](../../../../tests/ui/selectable_list/README.md).
No compilation/runtime reproduction or passing runtime correction is claimed;
M07-06 and this correction await lead validation before M07-07.

[Step](step_06_product-validation.md) · [Inventory](application-inventory.md) ·
[Progress](../progress.md)

## Baseline and evidence policy

Starting revision: `f7c2498cb4c29e27921c91809043dcf5fe4d963c`, with inherited
M07-03–05 source and documentation changes preserved. M07-05 is accepted by
project lead continuation instruction. Compilation/checks are assumed successful
under the execution convention; specific backend, scenario and package coverage
is not specified. This record is source-level product validation, not an observed
native application or package run. No build-producing command was executed.

## Workflow reconciliation

The initial inventory table describes M07-01 gaps. M07-02–05 handoffs supersede
those labels; the following maps current implementations and remaining checks.

| Workflow | Source-level implementation/evidence | Project-lead regression |
|---|---|---|
| New/open/close/save and unsaved state | `DocumentIo.cpp`, `AppDelegate.cpp`, `StageScene.cpp`; queued replacement, canonical scene paths, explicit draw DB overloads and document generations | Save/Discard/Cancel on open/new/quit; invalid input/destination retains a usable scene; single/multi-root hierarchy and resources round-trip after restart |
| Import/prefab/export selected | `DocumentIo.cpp`, `ImportActions.cpp`, `StageScene.cpp`; unit/axis wrappers, selected parent, complete selected-model export and explicit per-item failure boundary | BG2/OBJ/MTL/glTF/GLB/prefab insertion; export excludes descendants/placement as documented; collisions, missing textures and protected inputs |
| Hierarchy/components/cloning | `SceneEditor.cpp`, `EditorActions.cpp`; weak queued intents, enabled-camera/eight-light validation, independent clones, component staging and rollback | Ctrl selection, reparent/cycle rejection, original-versus-clone independence and serialized component/camera state |
| Chain controls | Production `SceneEditor::drawChainComponentControls` compared with draw inspector; M07-06 restores missing add/remove controls using `ChainComponent` and existing queued mutation hooks | Add once, remove through Components, duplicate/save/reload, chain-joint transform behavior and stale queued selection |
| Materials/selection/tools | `EditorUi.cpp`, `EditorTools.cpp`, `EditorActions.cpp`; typed DrawMaterialEditor, material clipboard/modifiers, node/submesh intents and helper exclusion | Picks/gizmo capture, multi-selection, texture/sampler/UV edits, partial failure dirty/history notification and independent clones |
| Cameras/viewpoint/environment | `StageScene.cpp`, `EditorTools.cpp`, `SceneEditor.cpp`; active authored camera, preserved projection, stopped analysis and environment candidate publication | Static and orbit cameras, parent conversion/rejection, camera switching, invalid HDR replacement and zero/eight-light policies |
| Async/HTTP/assets | `SceneImporter`, `ImportServer`, `ImportActions` and `ImportUi`; CPU-only joined readers, bounded jobs, independent preferences and guarded publication | Queue/cancel/reimport, status/error codes, occupied port/slot, document replacement, failed rollback and exit during parsing/HTTP wait |
| Graphics configuration | Live facade/GraphicsSettingsWindow and separate transient diagnostics in `EditorUi.cpp` | Settings JSON/preferences round-trip, rebind/stale handles, default resolution policy, no-RT raster fallback, supported RT and optional FSR/SDK unavailable paths |
| Integrated baking | `EditorBaking.cpp`, `SceneLightmapWindow.cpp`; selected-subtree/scene serial jobs, all results captured before adoption, per-target previews and complete-save sources | AO/GI, Warn overlap, cancel/edit/import/rebind, RGB8 versus HDR, finite display limits, multi-submesh metadata and complete save/reload |
| Workspace/dialogs/shutdown | `EditorUi.cpp`, `AppDelegate::cleanup`, shared native dialogs and MainLoop | Dock/menu/panel reopening, file drop/dialog cancellation, UI input capture, retire bake then join workers/server and release resources before Engine teardown |

No known missing Chain add/remove feature is deferred to retirement: its correction
is part of this step. Arbitrary custom component cloning/resource export still
requires the registered component contracts; the inspector deliberately stages
known lightweight CPU components rather than cloning unknown GPU-owning objects
on every UI draw. Custom extension scenarios need explicit product checks, not an
inference that all registered types have a complete editor UI.

## Read-only distribution inspection

- `apps/CMakeLists.txt` registers both production and draw Composer separately.
  `apps/draw/bg2e_composer/CMakeLists.txt` creates `draw_bg2e_composer` through
  `bundle_app_sdl`, with independent macOS bundle identity.
- `cmake/utils.cmake` links the SDL application to the engine and copies the shared
  engine shader and asset roots. Paired draw/bake bundle refresh uses the existing
  aggregate shader dependency. Composer declares no separate app shader tree.
- `cmake/package_apps.cmake` recursively discovers executable targets beneath
  `apps/`. Distribution includes registered apps, platform runtime handling and
  resources; no Composer-specific allowlist addition is required. On macOS it
  installs bundles; other platforms install executables and shared resources.
- Platform packaging remains governed by `scripts/PACKAGING.md` and the native
  generator constraints. No packaging script, CMake configuration, install or
  build was run; no CMake file was modified. Native dialog dependencies and
  installed/bundled resource resolution require actual platform execution.

## Intentional differences and limits

Draw owns backend-neutral resources and live configuration, not production Vulkan
frame APIs. Diagnostic settings are transient. Tree selection is node-only with
separate material/submesh intent; reparent preserves local pose. Camera and light
validation, static-camera framing limits, parser subsets and synchronous stopped
GPU realization are explicit in the implementation handoffs. HTTP defaults may
conflict when both apps bind port 8643; failure reporting/port change is intentional.

Bakes retain all outputs until adoption and use completed-frame drains; this is
not a throughput-optimized standalone batch. Temporary applied sources survive
until complete save. Material adoption may partially succeed on setter/tool
failure; dirty state and referenced sources are preserved, not claimed rolled
back atomically. These limits must remain visible in M07 review and M08 output
design; they are not permission to remove a production workflow without review.

## Project-lead execution matrix

All rows currently have **unspecified scenario/platform coverage**. Continuation
acceptance of M07-05 is recorded separately and is not package/backend evidence.

1. Build/run `draw_bg2e_composer` on available Vulkan and Metal hosts; exercise
   no-RT raster editing, requested/supported GI/reflections and optional FSR.
2. Run the workflow rows above, including Chain correction, then the focused
   [IO](../../../../tests/draw/composer_io/README.md),
   [import/HTTP](../../../../tests/draw/composer_import/README.md) and
   [bake](../../../../tests/draw/composer_baking/README.md) matrices.
3. Produce a package using the documented native script from a suitable checkout;
   launch from its deployed directory/bundle without source-tree resource access.
   Check native dialogs, paired draw/bake shaders, assets and runtime libraries.
4. Exercise failure/close with queued edits, a running parser, waiting HTTP client
   and submitted bake work. No stale publication or Engine-after-cleanup access.
5. Launch production Composer and draw_model_edit independently; check legacy
   SceneTree behavior, shared DB/component clones and ordinary draw examples.

Agent checks: source declarations/call sites and ownership, Chain clone and
serialization hooks, full GPL/header/control-flow/function-separation style at
touched sites, resource target/helper/install routing, local Markdown paths and
working-tree/index whitespace. No application, parser, HTTP, shader, fixture,
package or GPU execution. No numerical/performance/visual parity is asserted.

M07-06 is delivered awaiting project-lead validation. M07-07 is the separate
code-versus-remaining-plan review; M07-08 remains the cumulative documentation
closure. M08/M09 product and retirement gates must retain the unspecified native
package/runtime coverage rather than treating this record as executed evidence.
