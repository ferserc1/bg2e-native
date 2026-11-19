# Step 08: Complete instance editing, replacement and the small gizmo

## Prerequisite

Step 07 implemented and buildable; record its user validation status.

## Work

Expose undoable independent submesh material, transform and visibility editing; duplicate instances without cloning geometry. Hide/disable all geometry-modifying actions for instances, including indirect menu/selection command paths. Expose owner replacement and standalone conflict diagnostics. Review reflection and editor actions after the Text inheritance change.
Add GizmoType::DrawableInstance using two small offset wireframe cubes, screen-size-aware, hidden by default, with its own visibility/scale/opacity settings. Exclude helper geometry from model cache, final rendering and bake. Preserve transform gizmo behavior.
Integrate undo/redo material retention and geometry replacement revisions without multiplying memory accounting for shared buffers/BLAS.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

User validates editing isolation, owner replacement propagation, topology reconciliation, duplicate/delete/undo/redo, text regression and gizmo toggles.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 09-cleanup.md
