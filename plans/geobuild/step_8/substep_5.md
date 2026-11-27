# 8.5 Edit surface groups with pastel previews

## Entry condition

[8.4 Connect transform gizmos, face handles and grid](../step_8/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer workspaces/volume material tool and geobuild grouping/picking.

## Implementation contract

Switch tools only on a current valid mesh revision. Hide operand overlays. Show one stable muted pastel per group and a neutral default; draw logical boundaries and a distinct full-region hover/selection outline. Show matching group swatches. Allow click, additive multi-selection, create/rename/delete group, assign/reassign and clear membership. One selection may span nonadjacent or differently oriented regions. Reuse existing material editing only on the published Drawable. Group-only edits preserve UVs. Returning to geometry keeps the same session and undo stack.

## Acceptance evidence

Confirm separated fronts can receive distinct groups and later share a group. Curved UV seams do not fragment selection. Ensure colors are transient and never copied into output materials.

## User validation

Assign different colors to both wall fronts, group one with a jamb, move them again and undo across submodes. Judge pastel legibility and hover boundaries before accepting.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.6 Apply, cancel and save the active draft](../step_8/substep_6.md)
