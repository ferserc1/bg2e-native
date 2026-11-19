# Step 02: Implement the device-backed drawable cache and resource identity

## Prerequisite

Step 01 implemented and buildable; record its user validation status.

## Work

Add draw::DrawableCache associated with draw::Engine/device, usable without a Scene tree or UI. Introduce stable resource handles, original CPU submesh definitions, geometry revisions, source/export path metadata and entry enumeration. Keep draw independent of db/scene: DB adapters perform loading.
Define load/find/registerProcedural/registerImported/replace/queryUsage/canUnload/unload operations and structured recoverable diagnostics. Procedural IDs are unique; imported IDs use the source filename plus a unique suffix; BG2 lookup uses normalized absolute paths. Keep display name, stable identity, lookup aliases and export destination separate, especially across Save As.
Retain strong resident ownership. Define separate accounting for scene owners, scene instances, history retention and active jobs/frames; do not infer these from shared_ptr::use_count(). Add Drawable usage queries backed by lifetime-safe registration, not public counter manipulation. Reject cross-device binding safely. Preserve original definitions independently of Drawable cleanup snapshots.
Coalesce repeated BG2 requests and leave no half-published entry after failure. Keep native debug names meaningful and nonempty. No persistent cache JSON yet.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No new controls. Cache plumbing must leave current scene behavior intact.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 03-instance-component.md
