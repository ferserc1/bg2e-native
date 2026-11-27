# 8.3 Wire viewport selection and full-gesture ownership

## Entry condition

[8.2 Edit and preview an ordered list of operands](../step_8/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer AppDelegate mouse handlers, workspaces/volume and geobuild queries.

## Implementation contract

Adapt the existing mouse down/move/up path by active workspace and submode. Retain scene input in Scene. Give UI capture, active handles and existing navigation gestures their established priority. Route the entire gesture to its initial consumer until release/cancel/focus loss. In geometry mode show hovered operand in viewport and list; successive clicks within the existing click tolerance at the same point cycle the ordered candidate list. Reset cycling when cursor, camera, candidate list or revision changes. Keep the active operand selection separate from scene selection. Use an existing scene occlusion query if it gives a comparable distance; otherwise intersect ordinary scene geometry in Composer at click/hover time, excluding builder preview/owner. Keep external scene traversal outside geobuild.

## Acceptance evidence

Verify picking through the builder wall to a cutter, occlusion by an imported foreground mesh, list access to occluded operands and no scene selection side effects. A drag ending over another handle must not cycle or change selection.

## User validation

Click repeatedly through overlapping operands, hover the matching rows, pick behind the wall, then orbit on top of a handle. Check that hover, click, orbit and drag remain distinguishable.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.4 Connect transform gizmos, face handles and grid](../step_8/substep_4.md)
