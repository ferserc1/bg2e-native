# Step 1 — Mutation inventory and contracts

Dependency: none. Read [summary.md](summary.md) and [progress.md](progress.md) first.

## Objective

Map actual edit paths before introducing a reusable command contract. This is implementation design work, not a manual application test plan.

## Work

- Inspect all Composer sources, not only `EditorActions.cpp`: input/gizmos, submesh metadata, scene graph, reflection actions, environment, camera tools, import adoption, UV generation and lightmap publication.
- Record the inventory in `mutation-inventory.md` beside this file. For each action identify its source location, origin, destination identity, document impact, recording policy, execution boundary, restoration strategy and invalidation effects.
- Classify navigation, runtime component updates, selection, auxiliary tools, authored edits, file IO and prepared-result insertion. Identify mutations currently performed by shared SDK widgets/tools.
- Define a concrete command lifecycle: pending, executing, applied, undone, discarded and faulted as needed. Keep execution ownership separate from history ownership.
- Specify the reusable API contracts: command results/no-op handling, undo capability, category, recording policy, document impact, origin context, composite behavior, estimated retained bytes, queue completion and already-applied adoption.
- Specify effective-policy precedence: runtime/navigation/history replay cannot accidentally record; category preferences resolve only `UserPreference`; reject required recording without undo support. Snapshot the effective choice at dispatch/transaction start.
- Define compatibility of unrecorded actions with redo. Selection/navigation normally preserve redo; target destruction/replacement must not replay against a new object. Explicitly address changing selection-history preferences with existing entries.
- Define history state identity and dirty tracking for document edits, selection-only entries, pending transactions and persisted nonhistorical changes. Preserve save-path metadata outside scene commands.
- Identify ownership of shared drawables/materials, detached components, temporary image files, engine lifetimes and plugin-provided components. Retained objects/callbacks must not outlive their plugin module.
- Define handling when animation/controllers and editor input write the same property. Preserve runtime independence; document the chosen pause/base-value/conflict behavior instead of assuming undo will remain visible.

## Completion

The inventory covers every discovered scene mutation and names its planned command or explicit exclusion. Required API and failure contracts are settled, and implementation gaps are recorded in `progress.md`. Do not change CMake or compile.
