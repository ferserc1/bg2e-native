# 7.3 Pick logical regions on the evaluated mesh

## Entry condition

[7.2 Pick all intersected original operands](../step_7/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild immutable evaluated mesh and triangle-to-region table from step 5.

## Implementation contract

Expose a single cursor-to-logical-face query. Use the nearest visible surface of the exact displayed revision, including subtraction interiors. Resolve the indexed triangle to a logical region and return revision, region ID, hit position and comparable distance. Use deterministic edge ties within geometric tolerance. Reject hits past an optional external occlusion distance. Use CPU intersections; add a shared acceleration structure only if measured scene complexity warrants it.

## Acceptance evidence

Pick both halves of a divided wall, the jamb, a curved chart seam, an empty volume and an edge. Regrouping a region must not change hit geometry. Reject or refresh results from obsolete revisions.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [7.4 Resize primitives using opposite-face anchoring](../step_7/substep_4.md)
