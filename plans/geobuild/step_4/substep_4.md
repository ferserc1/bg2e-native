# 4.4 Evaluate the ordered boolean sequence

## Entry condition

[4.3 Construct closed primitives with source labels](../step_4/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild evaluator, pinned Manifold API and the primitive adapter.

## Implementation contract

Fold left through union, subtraction or intersection. Operate in builder-local coordinates; preserve double precision through solid processing. Read triangle runs and source face IDs into an immutable CPU result with positions, indices, adjacency inputs and source patch per triangle. Preserve the outward orientation of subtraction walls. Distinguish empty success from invalid input or library error, identify the offending operand, and never publish a partial result after failure. Keep this callable outside Composer.

## Reference fixture

Use a centered box of width 6, height 3 and depth 0.3. Subtract a centered box of width 1, height 4 and depth 1. The extra cutter height and depth avoid coincident faces. The remainder consists of two disconnected solids; their front patches have the same source side. For the window variant use height 1 with the same through-depth. Its front patch has one connected component. Test coplanar contact separately so failures in that case are distinguishable from provenance errors.

## Acceptance evidence

Exercise a wall minus through-cutter, overlapping union, disjoint intersection, identical operands, reordering and a later union following an empty intermediate. Verify jamb triangles originate from the cutter and normals face into the opening.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [5.1 Extract independently selectable logical faces](../step_5/substep_1.md)
