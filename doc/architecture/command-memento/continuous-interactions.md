# Continuous interactions

## Explicit boundaries

`EditInteraction` groups compound widgets, including popup controls, without a
time-based debounce. `EditSession` reports Begin, Update, Commit and Cancel. A
UI frame brackets the complete editor with `beginFrame/endFrame`; a control not
seen cancels its preview. Proposed values own their CPU data. Rejecting Begin
must leave the model unchanged. If cancellation fails, keep the session alive
for recovery.

The core transaction captures the original state once, budgets each preview
before update, restores the preceding preview on failed update, then either
adopts the final value or restores the initial value. Queue updates remain FIFO
except the narrow absolute-value coalescing rule in
[execution and history](execution-and-history.md).

## Material controls

Composer maps a material edit to `MaterialEditIntent` and freezes
`MaterialEditTargets` at Begin. The command stores a before definition and
candidate; updates replace the candidate. Commit adopts the result as one entry.
Without command history, a widget consumer can keep its mutation path or connect
`onEditRequested` to another owner. `onChanged` is post-mutation notification,
not intent.

## Gizmo adoption

Composer captures selected nodes' local matrices when the gizmo interaction
starts. During movement it pauses known orbit/polar camera writers. At the
graphics boundary, commit records before and after matrices without applying the
preview twice. Cancel restores before. Fixed/custom writers that cannot be
paused reject. Selection or document replacement settles the interaction before
rebinding. Targets are weak and membership is revalidated.

## Settlement

Before history requests or incompatible discrete edits, Composer settles
buffered drafts: valid drafts commit, invalid drafts cancel. Gizmo settlement
runs at the graphics boundary. History requests queue behind prepared drafts so
undo cannot race an edit displayed by UI but not yet published.
