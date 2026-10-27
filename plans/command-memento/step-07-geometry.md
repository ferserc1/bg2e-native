# Step 7 — Geometry commands and inverse operations

Dependency: step 6.

## Work

- Replace geometry operation integers/opaque modifiers in Composer with named commands under `command::geometry`: put on floor, center, axis conversion and unit scaling, plus other discovered geometry edits.
- Compute each forward transformation once from the initial geometry. Store its valid inverse; undo applies the inverse and redo reapplies the original forward operation. Never recalculate bounds or use the current selection to redo.
- Prefer inverse representations whenever the full operation is reversible. Check singular/non-finite transformations before mutation. Address positions, normals, tangents and winding where affected; inverse position transforms alone are not sufficient for every modifier.
- Document the accepted floating-point accumulation limitation. Avoid claiming exact bitwise restoration from inverse matrix operations.
- For information-losing operations, including applicable UV/topology operations, retain old/new CPU data or geometry/buffer representations. Preserve shared-resource semantics and selection/submesh validity across restoration.
- Deduplicate shared drawables/mutable mesh storage so each action transforms the intended geometry once. Specify whether edits affect all instances sharing that resource; preserve current behavior unless an explicit copy is required.
- Perform required GPU rebuilds for execute/undo/redo at safe boundaries. Batch invalidation/rebuild work where the existing SDK permits it without introducing unrelated renderer changes.
- Capture enough recovery state or prepare candidates to satisfy atomicity if GPU rebuilding fails after CPU mutation. Operation inverses do not by themselves guarantee successful resource rollback.
- Account for retained fallback geometry in the history budget; ordinary inverse commands should not retain full mesh snapshots without a stated recovery need.

## Completion

Reversible geometry commands store compact forward/inverse operations, irreversible edits have explicit retained-state fallback, and GPU ownership/failure paths are recorded. No Composer tests, compilation or benchmark tasks.
