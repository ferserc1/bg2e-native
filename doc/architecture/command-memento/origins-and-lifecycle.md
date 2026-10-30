# Origins and lifecycle

## Origin policy

Only explicit editor intent enters history. Runtime component updates and
animation alter evaluated state repeatedly and are not editor mementos.
Navigation (orbit, pan, zoom) is intentionally excluded even if a preference is
changed. Editing camera properties or moving its node deliberately through the
inspector/gizmo is an ordinary document command. This origin rule is a policy,
not missing command coverage.

Nonhistorical persisted changes need an explicit compatibility decision. A
preserved navigation update may keep history; an incompatible update invalidates
redo or resets history. Document save identity changes only after successful IO.

## Generation and teardown

Generation protects against valid-looking pointers that belong to an old
document. Async imports/bakes carry generation plus request/job tokens; both are
needed because a request can be superseded within one document. Initial
prepared-result validation is not reused for undo/redo, but target membership
and resource compatibility are rechecked.

On replacement/shutdown Composer settles drafts and previews, cancels queued
producers, waits for command completion, and drains renderer work before releasing
retained resources. Plugin leases keep code loaded while a command's virtual
destructor or callback may run. Deferred resource release must finish before the
device/engine owner is destroyed. These are caller-owned lifecycle boundaries;
the generic app core cannot infer a GPU fence.

## Selection preference

`recordSelection` defaults off. When enabled, future selection actions can be
undone without dirtying authored scene state. Disabling it does not erase
existing selection entries or change their policy snapshot.
