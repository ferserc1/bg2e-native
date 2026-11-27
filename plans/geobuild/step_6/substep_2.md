# 6.2 Serialize the versioned recipe and surface assignments

## Entry condition

[6.1 Attach and register the C++ component](../step_6/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

GeometryBuilderComponent serialization, lib/src/bg2e/db/scene*.cpp and existing scene/component serialization patterns.

## Implementation contract

Write schema version 1, ordered primitives (stable ID, label, kind, operation, affine matrix and tessellation), next ID counters, ordered group IDs and labels, and committed region-to-group references. Use stable textual enum values and the existing scene matrix layout. Encode large IDs losslessly (decimal strings if JSON numbers cannot represent them exactly). Serialization stores local authored data; positions of the owning node remain part of the scene. Validate versions, cardinalities, finite values, duplicate IDs and dangling group references when reading. Load into a temporary definition and publish only after validation. Retain baked rendering and report an unusable recipe using the existing scene diagnostics when feasible.

## Version 1 field dictionary

Use the standard component envelope and its type field. Inside it store schemaVersion, primitives, groups, assignments, nextPrimitiveId and nextGroupId. A primitive contains id, name, kind (cube, cylinder, sphere), operation (seed, union, subtract, intersect), transform and shape-specific tessellation. A group contains id and name. An assignment contains groupId, primitiveId, patch and the region signature from step 5. Use array order for operations and for group order; preserve IDs when reordering. Derive absence of a group from absence of an assignment. Exclude active selection, hovered region, grid step, camera, temporary native IDs, preview colors and workspace state. Keep actual material properties in the ordinary drawable pipeline.

## Acceptance evidence

Round-trip with nonuniform transform, a deleted ID, disconnected regions with different assignments, an empty recipe, and a malformed future version. A save-load-save with unchanged geometry preserves authoring selections, group ordering and materials.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [6.3 Publish the recipe and owner as one change](../step_6/substep_3.md)
