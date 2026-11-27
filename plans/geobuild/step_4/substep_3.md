# 4.3 Construct closed primitives with source labels

## Entry condition

[4.2 Integrate a pinned private Manifold dependency](../step_4/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild private primitive adapter; compare lib/include/bg2e/geo/{cube,cylinder,sphere}.hpp and the pinned MeshGL API.

## Implementation contract

Produce indexed, consistently wound, closed triangulations with shared topology or explicit vertex merge maps. Keep render seams out of the solid topology. Set fixed reproducible defaults of 32 cylinder segments and sphere 32 longitude by 16 latitude divisions, with shared poles. Store actual tessellation in the recipe. Map planar source facets to semantic source patches: six signed cube sides, cylinder caps and lateral surface, sphere exterior. Give distinct input primitives distinct original IDs per evaluation, including duplicates of a canonical shape. Keep the map back to persistent operand ID.

## Acceptance evidence

Check each undirected edge has two opposite halfedges, no degenerate triangles at poles/caps, bounds, orientation and primitive counts. Verify status and source labels after transforms, including a negative determinant.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [4.4 Evaluate the ordered boolean sequence](../step_4/substep_4.md)
