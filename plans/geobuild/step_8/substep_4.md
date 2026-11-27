# 8.4 Connect transform gizmos, face handles and grid

## Entry condition

[8.3 Wire viewport selection and full-gesture ownership](../step_8/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer workspaces/volume, TransformInteractions.cpp, SDK dimension utility and common snapping preferences.

## Implementation contract

Reuse general translate/rotate gizmos with a private proxy node for the active primitive. Route proxy changes to the draft only; use a dedicated Composer adapter instead of inserting helper components in the authored tree. Keep checks on unknown transform writers. Recognize the builder component as a non-transform writer on the ordinary owner when appropriate. Use dimension handles instead of generic scale on operands. Numeric inputs act on the same dimensions and anchor. Handle updates from the gesture baseline, one draft undo per completed drag, cancel on capture loss.

## Acceptance evidence

Check grid toggle/step shared between Scene and Volume, transformed parent, instance unaffected during preview, and exact anchoring of both positive and negative-side handles.

## User validation

Against an imported machine, move one jamb by two grid increments, enter an exact final width, undo, then resize a turned cylinder and sphere. Verify the opposite limit stays put.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.5 Edit surface groups with pastel previews](../step_8/substep_5.md)
