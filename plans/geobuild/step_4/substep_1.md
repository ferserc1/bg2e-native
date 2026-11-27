# 4.1 Define the editable CPU recipe

## Entry condition

[3.2 Integrate increments with the scene translation gizmo](../step_3/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

New C++ geobuild headers/sources; inspect lib/include/bg2e/{geo,math,base}.

## Implementation contract

Define ordered primitive records with stable per-recipe IDs, label, kind, tessellation, local affine transform and operation. Use a centered canonical unit cube, unit-diameter Y-axis cylinder and unit-diameter sphere. Matrix encodes placement and dimensions; avoid competing scale fields. Store an operation for non-first records; first always seeds the accumulator. If a reorder/deletion creates a new first record it becomes the seed; when the old seed moves later use Union as its default. Empty list gives a valid empty result. Use monotonic IDs so deleting then adding cannot retarget a selection.

## Acceptance evidence

Cover add/remove/reorder/duplicate, stable ID vs row index, nonuniform scale, reflected transforms and invalid nonfinite/singular transforms. Retain valid affine shear supplied through the SDK. Public recipe types do not expose Manifold, renderer or UI types.

## User validation

Launch Composer, open a scene and confirm ordinary scene editing still works. The recipe has no dedicated application interface at this boundary.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [4.2 Integrate a pinned private Manifold dependency](../step_4/substep_2.md)
