# 2.2 Classify common and scene-only actions

## Entry condition

[2.1 Extract the existing scene workspace](../step_2/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer EditorUi action registration, workspaces/common, workspaces/scene and AppDelegate.

## Implementation contract

Register a single catalog with explicit Composer aliases for the scene and geometry-builder slots. Share Save, Save As, graphics settings and UI settings. Keep Open, import, scene structure, owner/instance modifications and scene-only panels scoped to Scene. Classify every existing action and floating window. Common commands retain existing availability predicates. Keep settings windows independent of scene-only panels. Reserve the volume slot and its separate preferences path.

## Acceptance evidence

Audit every existing ID against its registration and current menu path. Verify shared actions appear once. Ensure any hardwired keyboard paths for scoped actions obey the same active workspace policy.

## User validation

In Scene, check File and settings, disabled entries, keyboard shortcuts, and the toolbar customization list. Cross-workspace checks become visible in step 8.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [2.3 Coordinate transitions and clear history safely](../step_2/substep_3.md)
