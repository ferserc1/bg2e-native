# 5.3 Bake scene-aligned UVs into render vertices

## Entry condition

[5.2 Define group assignment and revision persistence](../step_5/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild mesh conversion and lib/include/bg2e/geo/{Mesh,Vertex}.hpp.

## Implementation contract

Use one fixed world frame, origin zero, one repeat per scene unit. Transform local positions to world only for projection. For each planar normal choose its sign canonically by largest absolute component (ties X,Y,Z), choose the least parallel world axis with the same deterministic tie order, build an orthonormal in-plane basis and project by dot products. Use a shared plane normal/basis over the full patch to prevent triangle-level flipping. For curved surfaces use connected projection charts clustered by dominant signed world axis of geometric facet normals; use that axis plane and a common scene origin, align adjacent chart offsets at a shared boundary anchor when possible. Accept curvature distortion and unavoidable seams. Split render vertices only where attributes differ. Keep UVs for regrouping; geometry regeneration uses the current scene frame.

## Acceptance evidence

Check equal world lengths on separate coplanar walls, a slanted wall with nonuniform scale, grid step changes, opposing orientation, and camera independence. Check finite chart UVs and seams on a pillar and sphere. UV0 is conventional; initialize UV1 by the existing mesh convention and preserve the dedicated lightmap workflow.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [5.4 Assemble the ordinary Drawable](../step_5/substep_4.md)
