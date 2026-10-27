# Step 8 — Prepared results and complete Composer integration

Dependency: step 7.

## Work

- Keep model/prefab parsing and resource preparation in existing import services. Publish prepared insertions through a command retaining nodes and placement. Undo removes the insertion; redo reuses the retained result rather than rerunning IO.
- Reject stale asynchronous completion using document/job/source revisions. Do not submit an obsolete job merely because its original target still exists.
- Keep baking computation/progress/cancellation outside the history. Apply completed lightmaps through one atomic command containing prior/new assignments, metadata and retained sources. Undo restores the previous assignments; redo does not rebake.
- Cover UV generation/replacement using the geometry fallback contract. Reconcile all existing partial-publication paths with command rollback/fault handling.
- Wire Undo/Redo menu actions and platform shortcuts, descriptive labels, availability, busy/fault reporting and category preference persistence. Respect text-control undo ownership so application shortcuts do not unexpectedly consume an active text edit.
- Resolve active valid transactions before undo/redo/save or another incompatible action. Define invalid-input cancellation and preserve dispatch order through graphics queue execution.
- Preserve redo across compatible selection/navigation. Invalidate or reject incompatible external structural mutation explicitly; never silently apply a command to a replacement target. Scene membership checks must allow a command's deliberately detached retained objects during restoration.
- Complete the inventory audit: every scene mutation is a command, a supervised interaction, prepared-result publication, or a documented runtime/navigation/editor-only exclusion. Replace remaining opaque scene-edit APIs or narrow them so Composer cannot accidentally bypass classification.
- Keep file lifecycle and auxiliary editor settings out of scene history. Ensure saved-state tracking accounts for persisted nonhistorical changes without marking selection dirty.
- Reconcile command ownership with document close, plugin cleanup and engine shutdown; clear retained histories before dependent modules/resources disappear.

## Completion

All inventory entries are accounted for, command menus/preferences are connected, and the code is ready for the user's Composer exercise. Do not add application tests, manual application checklists or a testing phase. Update `progress.md` with source-review limits.
