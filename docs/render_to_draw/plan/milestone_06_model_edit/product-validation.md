# M06-06 — Product validation record

[Step](step_06_product-validation.md) · [Feature inventory](application-inventory.md) ·
[Progress](../progress.md) · [Application guide](../../../../doc/apps/draw_model_edit.md)

Date: 2026-10-19. This record separates source/package inspection from runtime
evidence. The project lead owns compilation and native application checks under
the repository instructions; no runtime result is inferred from static inspection.

## Static inspection completed

- `apps/draw/model_edit/CMakeLists.txt` registers `draw_model_edit` through
  `bundle_app_sdl`, the common helper used by SDL applications.
- `bundle_app_sdl` creates a macOS bundle where applicable, links the engine,
  Vulkan and SDL, copies engine shaders and assets, and applies the existing
  Vulkan resource handling. The target declares no private shader tree.
- `cmake/package_apps.cmake` recursively collects executable targets below
  `apps/`; distribution installs those targets, engine runtime dependencies,
  assets and shaders. No app-specific package allowlist needs an entry for
  `draw_model_edit`.
- The platform package scripts configure distribution builds with examples and
  tests disabled and `BG2E_PACKAGE_APPS=ON`. Their documented clean-checkout
  precondition prevents running them against this working tree, and the repo
  instructions prohibit compiling without an explicit request.
- The application guide and manual editor matrices describe native dialogs and
  drop, dirty prompts, failed-operation recovery, complete save/reload, material
  and geometry edits, UV2 regeneration, AO/GI cancellation/application, settings
  coexistence, and production editor coexistence.

## Project-lead execution evidence required

No application was configured, built, packaged or run by the agent. The following
remain without explicit scenario evidence. M06-06 was accepted by continuation;
this matrix retains the unreported checks for later product/retirement review:

| Area | Required evidence |
|---|---|
| Native development run | Launch and clean shutdown on available Vulkan and Metal hosts; no-RT fallback; resize, minimize/restore, input capture and UI window lifecycle. |
| Resource layout | Launch the installed/package executable with its deployed shaders/assets; confirm the draw/bake shader pair resolves from the package on each available platform. |
| File operations | Native open/save dialogs and file drop; BG2/VWGLB, OBJ/MTL, glTF/GLB and JSON/BG2SCENE/BG2E paths; cancellation and failure preserve the current document. |
| Document fidelity | Dirty Save/Discard/Cancel; edited model and graph save/reload; baked GPU-only texture sidecar and material metadata survive reload. |
| Editor parity | Workspace and menus; target/submesh selection, materials, texture previews, geometry actions, environment/lights, framing/best-view, UV2 and bake workflows. |
| Coexistence | Production `model_edit` still launches with its existing app identity and preferences; draw settings remain isolated. |
| Distribution | Native package script succeeds from a clean checkout, package launches, and executable/library/resource contents match the selected platform rules. |

The manual scenarios are maintained in [editor tools](../../../../tests/draw/editor_tools/README.md)
and [resource IO](../../../../tests/draw/resource_io/README.md). Add actual platform,
configuration, package path and observed outcome there or in this record when run;
do not infer backend coverage from the source-level package rules.

## Status

Static checks: `git diff --check` and `git diff --cached --check` pass. CMake and
package files were inspected read-only. Native generators, compile, shader build,
package scripts, executables and manual scenarios were not run. M06-06 remains
accepted by project-lead continuation with package/backend coverage unspecified.
The lead subsequently reports correct rendering without any lights. This is
specific evidence for that scenario, not a claim that the full matrix was run.
