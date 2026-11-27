# 7.2 Pick all intersected original operands

## Entry condition

[7.1 Build viewport rays in geobuild](../step_7/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild operand triangulations, viewport ray utility and immutable recipe revision.

## Implementation contract

Expose a single cursor-to-operands query. Broad-phase with operand bounds, then intersect the same actual triangle surfaces drawn by the preview. Include subtractive, fully enclosed and inactive operands. Return one result per primitive, nearest forward hit first, stable ID as tie-break; accept exits for rays beginning inside. A borrowed optional maximum scene hit distance clips against external occluders only. Return IDs, distances and the evaluated revision; UI cycling stays in Composer.

## Acceptance evidence

Try coincident operands, a fully enclosed cutter, a camera inside a solid and a miss inside a sphere bounding-box corner. A rotated and nonuniformly scaled operand must sort by world distance.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [7.3 Pick logical regions on the evaluated mesh](../step_7/substep_3.md)
