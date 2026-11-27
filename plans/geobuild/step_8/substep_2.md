# 8.2 Edit and preview an ordered list of operands

## Entry condition

[8.1 Enter and leave a volume editing session](../step_8/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer workspaces/volume inspector, geobuild recipe/evaluator and the existing renderer-safe update path.

## Implementation contract

Provide add/remove/duplicate/reorder, operation choices, numeric transform/dimensions and selection by list. Use a neutral solid preview, excluding the committed surface from duplicate drawing only in this editor view; restore it on exit. Render inactive operands with a faint translucent overlay and active/hovered operands with distinct contours. Show enclosed operands sufficiently for identification; other scene meshes remain references. Track draft and preview revisions; coalesce evaluations during dragging, refresh on release and discard stale results. Keep errors/empty results distinct and disallow applying stale output. Record reversible draft edits.

## Acceptance evidence

Construct wall + through-cutter + cylinder, change order, remove/undo an operand and check revision handling. Preview helpers must not affect independent instances or cast/bake into the scene.

## User validation

Build a wall and a subtractive opening. Reorder operations and switch selection by list. Judge whether the active cutter reads clearly and inactive outlines remain subtle against the imported model.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.3 Wire viewport selection and full-gesture ownership](../step_8/substep_3.md)
