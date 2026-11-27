# 5.1 Extract independently selectable logical faces

## Entry condition

[4.4 Evaluate the ordered boolean sequence](../step_4/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild evaluation output and topological adjacency.

## Implementation contract

Within each operand/source patch, flood-fill triangles across shared topological edges. Use manifold adjacency before splitting render vertices for shading, materials or UVs. Edge contact connects; a shared point alone does not. Keep touching regions with different provenance distinct. Create a triangle-to-region table and per-region triangle list on one evaluated revision. Cylinder sides and sphere exteriors may span many facets. A wall with a window has a connected front with an inner loop. A floor-to-ceiling cut yields two independent fronts.

## Region boundaries

Assign one pickable ID per connected component of the pair (operand ID, semantic patch). Flat sides of a source primitive remain distinct along a surviving crease; the wall on each side of an opening can belong to separate groups. If the backend removes a coplanar interior boundary, only surviving triangles with valid provenance participate. Group membership never feeds back into connectivity. Compute area and bounds once per region for diagnostics and highlight drawing.

## Acceptance evidence

Check those wall cases, adjacent coplanar operands where provenance survives, point contact, and curved patches. Boundaries must follow the surviving solid; no artificial selections for tessellation diagonals.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [5.2 Define group assignment and revision persistence](../step_5/substep_2.md)
