# 5.2 Define group assignment and revision persistence

## Entry condition

[5.1 Extract independently selectable logical faces](../step_5/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild groups, regions and recipe editing data.

## Implementation contract

Provide stable group IDs, display labels and creation order. Each region has zero or one group. Multi-assignment moves selected regions from prior groups; deleting a group returns its faces to default. Keep default implicit. Preserve assignments for unchanged geometry, material-only changes and reopening. Persist reproducible region references for the committed geometry: operand ID, source patch and canonical sorted local boundary edge coordinates (normalize endpoint order and signed zero; derive boundaries on the welded topology). Treat ambiguous or unmatched references after a geometry edit as unassigned; exact matching is sufficient. Do not depend on runtime Manifold IDs, triangle order, overlap tracking or irreversible island ordinals.

## Closed curved regions

A completely closed patch such as a whole sphere may have no boundary edges. For an empty boundary or another signature collision, use the canonical sorted local triangle coordinates within that source patch as a secondary signature, independent of index order and winding. Validate exact identifying data when resolving hash collisions. Use coordinates in the source operand frame. Persist only references needed to recover assignments; keep a version for the signature encoding. Identical committed input must recover assigned groups after reopening. After a changed evaluation, only an unambiguous match is reused.

## Acceptance evidence

Reorder triangle enumeration on identical geometry, save/recreate equivalent input, assign the two wall fronts separately and together, clear one and delete a group. Topology changes may require reassignment. A stale revision cannot select a current face.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [5.3 Bake scene-aligned UVs into render vertices](../step_5/substep_3.md)
