# M06 cumulative documentation coverage

[Plan](../index.md) · [Progress](../progress.md) · [Technical review](review.md) ·
[Documentation step](step_08_documentation.md)

Starting/ending HEAD: `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`.
Inherited staged, unstaged and untracked work is preserved. M06-07 and subsequent
editor corrections were accepted by project lead continuation. The lead explicitly
reports correct rendering without lights; unspecified backend/package coverage is
not inferred from that report. No renderer consequences of zero lights were
analyzed in this step. This delivery changes documentation only.

## Changed and reconciled maintained pages

| Pages / disposition | Actual source and delivered behavior |
|---|---|
| [Application guide](../../../../doc/apps/draw_model_edit.md), updated | `apps/draw/model_edit/src/{EditorUi,EditorTools,EditorActions,EditorBaking,StageScene}.cpp`: Workspace/menus, targeted selection/material/geometry operations, environment/light reflection, light-only gizmos and automatic bake application. Initial/random coordinates and removal of every light are explicit. |
| [Application tutorial](../../../../doc/tutorials/draw/model_edit.md), new | Sequential delivered-product workflow: launch/resources, open/select/edit, environment and focused components, graphics/debug separation, UV2/bake/retained preview and complete save/reopen. It is not a new numbered C++ example or future Composer tutorial. |
| [Tutorial index](../../../../doc/tutorials/draw/index.md), [app index](../../../../doc/apps/index.md), updated | Link the actual model editor workflow without assigning an unimplemented example target. Numbered tutorials 01–13 remain the rendering-example series. |
| [GraphicsSettingsWindow](../../../../doc/api/ui/GraphicsSettingsWindow.md), [UI index](../../../../doc/api/ui/index.md), [UI reference](../../../../doc/api/ui/reference.md), updated | `GraphicsSettingsWindow` and `RendererDebugWidget` declarations/implementation: shared live facade and queued edits, graphics-only UI, separate diagnostic/capability window, no debug persistence controls. |
| [Draw settings](../../../../doc/api/draw/Settings.md), updated | `SettingsCatalog::{isGraphicsSetting,isDebugSetting}`, `SettingsCodec::{serialize,validate}` and `RenderSettingsPreferences`: scene targets stay accessible to explicit catalog clients but are excluded from graphics preference documents. |
| [Complete resource IO](../../../../doc/api/db/DrawResourceIO.md), updated | Draw-aware DB overloads, `draw_resource_io.cpp`, `texture_archive.cpp`: actual GPU snapshots, format/mips/HDR, material metadata, final sidecars and transaction publication. Temporary bake files do not remain saved-document dependencies. |
| [MainLoop/render/draw architecture](../../../../doc/architecture/MainLoop_render_draw_gpu.md), updated | App-owned document guards, stopped mutations, reflected component clones, bake job/result ownership, fixture exclusion and complete export. |
| [Reflection widgets](../../../../doc/api/ui/Reflection_Widgets.md), [Numeric](../../../../doc/api/ui/Numeric.md), retained after source check | `ReflectionWidget.cpp`, `Numeric.cpp`, `Vector.cpp`: unbounded scalar/vector/matrix rows use drag with text entry; bounded angles retain their editors; UInt/Double drag uses native types. These pages were updated during accepted corrections. |
| [Editor README](../../../../apps/draw/model_edit/README.md), updated | `main.cpp`, `EditorUi.cpp`, app identity, CLI backend selection, shared bundle resources and actual integrated bake status. Removed stale startup-only/future-bake wording. |
| [Manual editor checks](../../../../tests/draw/editor_tools/README.md), updated | Source-owned manual scenarios now include automatic application, retained map preview, settings persistence exclusions, focused light editors and light removal/positions. This is a checklist, not new execution evidence. |

## Retained cumulative implementation baseline

The M01–M05 documentation baseline remains valid with the corrections above:

- `doc/api/draw/` renderer/delegate/effect/environment APIs, deferred renderer and
  material references; tutorials 10/11 keep public injection, scene extraction,
  selection and supported effect contracts. Explicit catalog light clients remain
  distinct from graphics preference UI/serialization.
- `doc/api/gpu/` image/subresource/attachment/raw-transfer and capability contracts:
  native APIs stay inside backend implementations. This step adds no GPU facility.
- `doc/api/bake/` integrated context, executor, UV surfaces, preview and typed
  output: exact completion, linear RGB/HDR and image ownership remain unchanged.
  `EditorBaking.cpp` integrates those APIs; it is not a standalone scheduler.
- `doc/api/geo/{UvAtlasValidator,GenerateUv2AtlasModifier}.md` and
  `doc/architecture/lightmaps/`: nonfatal bake overlap warnings, structural failure
  distinctions, UV v=0 top-row orientation and Vulkan/Metal source-level mapping
  already reflect accepted corrections. No new visual backend proof is implied.
- `doc/api/db/index.md` already navigates to complete draw IO and reference-only
  IO. Scene/component reflection and base/math/geo foundations remain available;
  model editing does not expose a full scene component inspector.
- `doc/tutorials/draw/{12_lightmaps,13_offscreen_application}.md` and
  `doc/architecture/{draw_integrated_lightmaps,draw_offscreen_rendering}.md` retain
  their own delivered example scopes: tutorial 12 has direct in-memory adoption;
  offscreen tutorial 13 exports a camera PNG rather than complete model resources.
- Workspace/window/input/safe-update guides retain shared and production contracts;
  the new app guide identifies the actual draw-specific orchestration. Production
  application guides and `bg2e::render` documentation stay truthful through M08.

No maintained page was retired in M06. Production guides are retained as working
references; deletion/promotion belongs to M09, not documentation closure.

## Build, resources and distribution

Read-only review of `main.cpp`, the existing app registration, shared SDL bundle
helper and [packaging guide](../../../../scripts/PACKAGING.md) confirms the
registered target and shared draw/bake shader/assets/runtime routing documented by
[M06 product validation](product-validation.md). No CMake or packaging script was
edited or executed. The application tutorial states platform generators, Vulkan
SDK requirement, macOS Metal scope and deployed resources rather than implying an
isolated executable is sufficient. Standalone project/library update scripts
retain their existing shared-engine workflow; no app-specific variant is needed.

## Checks, limitations and remaining owners

Static checks cover local Markdown links/anchors, fenced blocks, referenced source
and header paths, CLI spellings, native/archive suffixes, actual component/settings
symbols and both working-tree/index whitespace. Tutorial commands and GUI steps
were source-checked, not run. No configuration, compilation, shader compilation,
C++ probe, executable, package script or runtime test was run by the agent.

Native dialogs, complete imported-model/baked-map round trips, no-RT fallbacks,
platform-specific deployment and package launches remain project-lead checks with
coverage unspecified. Acceptance by continuation is recorded independently from
those missing details. The lead's zero-light rendering result is preserved as a
specific positive report.

M07 owns Composer graph/component editors, async/server import and multi-target
scene baking documentation when implemented. M08 owns standalone context,
assembly/batch/preflight/output documentation. M09 owns canonical draw promotion,
legacy retirement and repository-wide distribution reconciliation. M10 audits the
accumulated maintained pages. None of those future products is documented here as
already implemented.

M06-08 is delivered awaiting project-lead acceptance. The next required step is
[M07-01 application inventory/copy](../milestone_07_bg2e_composer/step_01_application-copy.md).
