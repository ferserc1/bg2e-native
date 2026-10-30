# Extending command editing

## Add a command category

1. Identify the authored state and explicit user intent; do not hook a generic
   setter or post-change notification.
2. Choose `CommandInfo` name, category, history policy, document impact and
   undoability. Freeze any user preference at admission.
3. Capture stable target identity and the before value immediately before
   publication. Use weak destination guards plus document generation.
4. Define memory bounds for candidate, redo, rollback and retained resource
   versions. Reject if the bound cannot be reserved before mutation.
5. Prepare derived resources privately. Publish atomically, compensate partial
   external changes, and return `Faulted` if compensation fails.
6. Declare renderer/selection/tool effects at the application boundary. Keep
   queue transport separate from command semantics.
7. For continuous input, capture once at Begin, budget each Update, then commit
   adoption or cancel restoration. Do not use debounce as an interaction model.
8. For composites, require compatible child policies/category/impact and define
   child compensation order.

## Integrate another application

Construct `CommandHistory` on the model owner thread. Give `CommandQueue` an
executor that marshals one task to the appropriate safe boundary, and pump it
from the host loop. Keep history alive until queue shutdown and all completions
finish. Use a release sink only when native/module lifetimes require deferred
destruction; retain command and resource tickets together. Call `markSaved`
after successful IO. Increment generation when replacing the model, and stop
producers before baseline reset.

The UI SDK is optional: keep legacy widget behavior, use `EditSession` with your
own transaction owner, or submit typed edit intents to a different history
implementation. Notifications are not mutation requests. This separation keeps
Composer policy out of the generic app library.
