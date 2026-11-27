# 2.1 Extract the existing scene workspace

## Entry condition

[1.4 Isolate toolbar customization by workspace](../step_1/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

apps/bg2e_composer/src/{EditorUi,EditorActions,EditorTools,SceneEditor} and related setup/draw code.

## Implementation contract

Create workspaces/scene with a matching Composer namespace. Move ownership and setup of scene-only toolbar, panels, menu contributions and floating windows there in small behavior-preserving moves. Keep EditorUi responsible for application UI coordination. Initialize one scene workspace on the existing stage, preserve window titles, action IDs, preferences path composer-scene-tools.toolbar and existing shortcuts.

## Acceptance evidence

Audit initialization, resize, panel toggles, floating windows and lifetime of weak callbacks. No document reload on activation.

## User validation

Open an existing scene, toggle panels, use a scene command, customize the toolbar and restart. The layout and normal editing behavior should match the previous version.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [2.2 Classify common and scene-only actions](../step_2/substep_2.md)
