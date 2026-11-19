# Step 04: Integrate exclusive owners, replacement and scene lifecycle

## Prerequisite

Step 03 implemented and buildable; record its user validation status.

## Work

Register all authored DrawableComponent resources, including procedural and imported ones. Enforce at most one owning DrawableComponent per resource through constructors, setters, cloning, load and undo restoration. An invalid standalone assignment leaves the previous state intact and returns diagnostics for a UI message box; low-level code never opens UI.
A resource replacement by its owner updates the stable entry so all instances see the new geometry; reconcile instance state by index without replacing their authored materials/transforms. Preserve a distinct operation for rebinding one component. Owner cloning creates independent geometry and identity. Removing the owner leaves instances usable and preserves original export defaults.
Wire Scene/Composer context lifetime and DB loading adapters without introducing a global singleton. New/open documents receive a fresh cache; failed opening preserves the current document. Publish only complete replacement contexts, drain/retain GPU work correctly. Integrate history retention, memory accounting, clipboard and active offscreen jobs. Do not automatically unload zero-use resident entries.
Audit derived geometry producers/helpers; avoid treating generated text or overlay resources as editable model owners. Preserve existing node compatibility policy; if a new restriction is necessary, seek approval rather than inventing it.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No new controls yet. Existing load/import/duplicate/new/open behavior remains operational.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 05-scene-format.md
