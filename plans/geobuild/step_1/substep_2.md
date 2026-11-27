# 1.2 Project actions into menus and toolbars

## Entry condition

[1.1 Define workspace masks and active workspace](../step_1/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/src/bg2e/ui/{Toolbar,Menu,ToolbarSettings}.cpp and ActionDefinition.

## Implementation contract

Register one shared immutable action definition per stable ID in the collection. Build the eligible catalog per toolbar using membership. Keep menu path, dynamic label, enabled/checked callbacks and toolbar button layout independent. An empty path denotes a toolbar-only action. Apply separators after filtering and omit empty submenus. The customization picker must use the same eligible catalog. Validate duplicate IDs and malformed definitions before publishing any part of a batch.

## Acceptance evidence

Trace a scene-only Open, shared Save, toolbar-only action and a two-workspace action through menu, default layout and customization. Confirm the legacy toolbar registration route remains usable.

## User validation

Launch Composer and confirm that existing menus and toolbars continue to work normally.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [1.3 Dispatch workspace shortcuts exactly once](../step_1/substep_3.md)
