# Continuous interactions

## Explicit boundaries

`EditInteraction` groups compound widgets, including popup controls, without a
time-based debounce. `EditSession` reports Begin, Update, Commit and Cancel. A
UI frame brackets the complete editor with `beginFrame/endFrame`; a control not
seen cancels its preview. Proposed values own their CPU data. Rejecting Begin
must leave the model unchanged. If cancellation fails or is still pending, keep
the session alive and retry before rebinding or cleanup. `EditSession` retries
pending cancellation on later frames and prevents updates/commit overtaking it.

The core transaction captures the original state once, budgets each preview
before update, restores the preceding preview on failed update, then either
adopts the final value or restores the initial value. Commit never calls
`execute()` again. Returning to the original values produces no history entry.

## Composer scheduling and ownership

Composer preserves the widget interaction ID and document generation in a
`ValueEdit` admission record. Begin reserves exclusive CPU interaction dispatch
before its deferred renderer callback captures the baseline. Discrete commands
admitted afterwards wait until settlement. The first callback uses a
conservative boundary to stop any running bake; subsequent updates use
`enqueueCpuValueUpdate`. No live mutation runs during UI drawing.

Begin, Update and Commit acknowledge admission; actual failures are reported
through the application status. The renderer update list preserves their FIFO
order, so Commit follows the final Update. Cancel returns false until the
restoration callback completes; a subsequent call acknowledges completion.
The original transaction ID is checked for every deferred operation. Rejected
updates retain the preceding preview for cancellation; a queued Commit after a
failed update cancels instead of recording a failed draft.

The material guard permits the owning interaction to continue while its
transaction is active. Other interactions cannot acquire it. Each applied
preview and restoration invalidates render histories/lightmap sources and
requests a scene frame. Dirty state includes an effective active preview,
although it has not yet become a history entry.

## Material controls

`materialInteraction()` creates a `CommandTransaction` for CPU-only
`MaterialEditIntent` patches and freezes `MaterialEditTargets` at Begin. It
captures separate original and final value sets for each live material and its
stored `materialDefinition`. Colors, scalar factors, UV scales, channels and
other CPU flags update together at the renderer boundary. Missing fields
preserve each target's own values. Publication validates all targets first;
custom and ambiguously shared materials reject.

Only fixed CPU value snapshots are retained. Updates do not clone native
materials, load textures or accumulate one command per frame. Candidate patches
that replace texture resources reject. A failed effects callback restores the
preceding values; failed recovery faults history.

Texture sources, sampling changes, indirect interpretation, material paste and
prepared lightmaps continue through the discrete retained-state `EditMaterials`
command. Their drafts remain private until Commit. Without command history, a
widget consumer can keep its mutation path or connect `onEditRequested` to
another owner. `onChanged` is post-mutation notification, not intent.

## Reflected components and environment values

`EditProperty` also implements `CommandTransaction`. Begin captures the original
property and stage editing state. Update uses a temporary reversible adapter to
validate/apply the next proposal and retain only its final snapshot. Failures
restore the preceding preview, including stage state and effects. Property path,
subtype and value type remain fixed throughout the interaction.

Supported component values, including light colors, and environment skybox blur,
visibility, horizon/zenith/ground colors preview through this path. Environment
GPU data remains derived and is synchronized by the renderer. Environment image
replacement and presets retain their dedicated resource commands. Existing
transform-writer and custom-component restrictions still apply.

## Gizmo adoption

Composer captures selected nodes' local matrices when the gizmo interaction
starts. During movement it pauses known orbit/polar camera writers. At the
graphics boundary, commit records before and after matrices without applying the
preview twice. Cancel restores before. Fixed/custom writers that cannot be
paused reject. Selection or document replacement settles the interaction before
rebinding. Targets are weak and membership is revalidated.

## Settlement

Before history requests or incompatible discrete edits, Composer settles active
value interactions and buffered resource drafts. FIFO successors wait for the
pending Commit. At the stopped document IO/lifecycle boundary, cancellation is
drained and the UI acknowledgement is retried before saving, replacement or
cleanup. `interactionActive()` on the material editor and component inspector
reports retained UI ownership, including pending cancellation. A stale renderer
callback cannot silently drop an applied preview: unresolved restoration faults
history and blocks replay until repair/replacement.

Source and documentation checks do not establish runtime rendering correctness;
these changes have not been compiled or exercised interactively.


## Manual verification after building

- Drag material roughness or albedo with several selected submeshes: every
  target previews immediately, untouched fields remain independent, and release
  adds one undo entry. Undo/redo restores the exact initial/final values.
- Drag away and back to the starting value: no entry is added and a previously
  clean document becomes clean again.
- Cancel with Escape, focus loss or a hidden editor: the baseline returns and
  no entry is recorded. Pending cancellation must finish before a target rebind.
- Repeat with light color, environment colors and skybox blur. Verify both the
  viewport and the saved/reloaded authored values after commit.
- Request undo, save or selection change while dragging: settlement follows the
  final update, and a command queued behind the gesture is not lost.
- Verify texture replacement/paste still follows the discrete resource path,
  and stale targets, renderer replacement and memory rejection cannot leave an
  unowned preview or publish a partial multi-target update.
