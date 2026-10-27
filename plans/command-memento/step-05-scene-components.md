# Step 5 — Scene, selection and component commands

Dependency: step 4.

## Work

- Implement `bg2e_composer::command::scene` and `::component` commands for node/submesh names and metadata, enabled/visible state, creation, duplication, removal, reparenting, transforms and component properties/addition/removal/replacement.
- Retain original/new instances for structural changes. Preserve parent identity, sibling/component placement, priorities, serialization flags and active camera/environment references. Do not serialize components as the undo storage strategy.
- Audit attach/detach hooks and resource cleanup. Retention must keep sufficient CPU/resource state for reattachment; it must not execute a destroyed plugin's code or keep a resource beyond its engine lifetime.
- Preserve existing graph rules, supported enabled-light counts and authored camera requirements. Validate before publication and roll back whole multi-target edits on failure.
- Use property-level before/after values for supported reflected properties. Provide explicit adapters for properties/actions with side effects or insufficient metadata. Reject unsupported reversible editing visibly instead of silently losing undo.
- Integrate gizmo transactions as transform commands. Redo uses stored final transforms, not input replay. Keep automatic component updates outside the command stream.
- Implement selection commands retaining before/after selection, primary item and submesh identity. Resolve stale selections safely. Expose configurable selection recording with an explicit default in Composer preferences; default off preserves the initial editing-focused undo behavior.
- Mark selection entries as not affecting document state. Navigation/animation remain excluded regardless of that preference. Changing preferences does not rewrite existing history or transactions.
- Group one user action over multiple targets into one composite/history entry; deduplicate shared destinations where needed.

## Completion

Scene/component mutation inventory entries map to identifiable commands, structural undo retains identities, and optional selection entries do not change the saved-state calculation. No Composer tests; user application checks are outside this plan.
