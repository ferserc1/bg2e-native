# Step 4 — Optional UI and gizmo interaction integration

Dependency: step 3.

## Work

- Extend UI wrappers with optional begin/update/commit/cancel information using engine-owned types only. No public ImGui types/includes and no dependency from ordinary widgets to a command history.
- Preserve default behavior for SDK clients that do not install integration callbacks. Establish a single mutation owner per edit path; widgets and commands must not both apply a value.
- Extend `ReflectionWidget` intent handling with a stable property path, typed proposed value and interaction identity. Resolve nested paths again at execution; do not retain borrowed child pointers. Keep old immediate/handler use compatible where practical.
- Handle action buttons separately from properties. A reflected arbitrary action requires a dedicated reversible adapter or explicit restoration contract; an opaque callable is not automatically undoable.
- Add a pre-mutation optional material-editor intent path. Existing `onChanged` remains a notification, not the source of memento capture. Preserve queue safety and resource-update opt-ins for non-history clients.
- Implement sessions for text/numeric inputs, sliders, color popups and nested compound controls. Commit on the appropriate interaction boundary, cancel on Escape, and avoid a new history entry per frame/internal subcontrol. Use debounce only as a documented fallback.
- Expose optional gizmo begin/update/end/cancel hooks. Begin occurs before the first mutation. Keep direct manipulation functional without an observer; Composer supervises an already-applied transaction and adopts it once at commit.
- Capture all manipulated local transforms once, freeze targets, and serialize manipulation with other commands. Resolve selection changes, disappearing controls, focus loss and document replacement without leaving open transactions.
- Navigation and runtime controller/animation updates bypass recording. Inspector edits to camera/controller properties remain eligible. Implement the step-1 policy for concurrent controller/editor ownership without making ordinary component setters history-aware.

## Completion

UI and gizmo integration is opt-in, continuous interactions have explicit boundaries, and no setters or post-change notifications manufacture commands. Record SDK compatibility details for final documentation. No Composer tests or manual test procedures.
