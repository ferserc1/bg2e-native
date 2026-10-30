# Composer scene and components

## Structural retention

A node owns child nodes and components in ordered collections. `NodeEdit`
operations insert/remove/reparent/replace while retaining the exact removed
instances and original placement. `SceneRetention` supplies shared leases and
resource accounting. Undo restores the same object identities and sibling or
component ordering; it does not deserialize a lookalike. New document
generation invalidates commands targeting the old graph.

Component removal has the same detached-versus-destroyed distinction. Detached
objects remain alive through the command and stage retention pool, then are
released only after history no longer references them and native work drains.
Custom/plugin components without a safe retention/unload adapter reject.

## Scalar property and selection

`RenameNode`, `SetNodeEnabled`, metadata and reflected component-property
commands retain typed before/after values. They resolve weak identity and current
membership for each operation rather than holding a raw property pointer.
Selection history uses `SetSelection`; its policy is `UserPreference`, impact
`None`, and preference defaults off. Toggling preference changes future
admission only. Existing entries remain, and selection does not mark the scene
dirty.

The actual selection command's member initializer makes the policy explicit:

```cpp
_info{"Select scene objects", "selection",
    CommandHistoryPolicy::UserPreference, CommandDocumentImpact::None};
```

This excerpt is from [`SelectionCommands.hpp`](../../../apps/bg2e_composer/src/command/scene/SelectionCommands.hpp),
not a standalone class. The command captures before/after `SelectionState` and
restores it through the selection manager.

## Active references and replacement

Commands snapshot and restore selection plus known active camera/environment,
floor/fill-light and child-list relationships where applicable. Replace keeps
the old branch while publishing the candidate at the same slot. Publication
validates enabled-camera and light-count constraints and refreshes scene
references atomically. A stale destination or unsupported custom object rejects
before publication. If compensating a partially completed external publication
fails, the document history faults rather than reporting a successful undo.

Exact adapters exist for the listed Composer-known cases. Unknown native/custom
payloads and acceleration structures lack a safe byte/retirement contract and
are rejected; there is no opaque irreversible fallback.
