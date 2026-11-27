# 3.1 Define shared snapping calculations and preferences

## Entry condition

[2.3 Coordinate transitions and clear history safely](../step_2/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/include/bg2e/math, existing math utilities, Composer preferences and workspaces/common.

## Implementation contract

Define snapping in engine units, measured from the gesture start. For relative displacement round signed delta / step to nearest integer, half steps away from zero, then multiply by step. For absolute grid alignment round the moving world coordinate relative to scene origin, enabled only on a world-axis-aligned motion. Retain the original off-grid offset in relative mode. Store shared enabled/mode/linear step preferences in Composer; start disabled, relative, 0.05 scene units. Use finite positive steps and retain last valid settings on invalid input. Keep angular increments separate from linear units; rotational snapping is outside this linear delivery.

## Acceptance evidence

Verify 0.123 + one relative step = 0.173 at step 0.05, absolute alignment to 0.15 when dragged there, negative increments, reversals, and continuous mode. Freeze settings at gesture start.

## User validation

Launch Composer and confirm that ordinary translation remains continuous. Snapping becomes available through the translation gizmo in 3.2.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [3.2 Integrate increments with the scene translation gizmo](../step_3/substep_2.md)
