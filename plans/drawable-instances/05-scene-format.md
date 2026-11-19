# Step 05: Serialize instance properties and recover legacy duplicates

## Prerequisite

Step 04 implemented and buildable; record its user validation status.

## Work

Store an ordered submeshes array inside instance JSON with material, matrix and all applicable BG2-derived appearance/metadata. Restore defaults first, overlay serialized values by index, ignore surplus items, fill missing items from originals. Recoverable malformed fields use defaults with warnings; mismatch never throws or aborts scene loading. Preserve unresolved references and authored instance data when realization fails.
Implement deterministic legacy migration: first owner keeps a normalized source path, later owners receive independent Drawables, identities and unique BG2 destinations. Attempt non-destructive physical copies during load; permission/copy failure leaves an independent in-memory copy and pending destination, not a cancelled scene. Resolve imported/merged scenes, cloned owners, undo restoration and Save As collisions consistently. Same-path instances reuse a resource; owner copies never silently share writable geometry.
For ambiguous manually edited references, resolve to the first resource retaining the original path and warn. No complex inference. Extend reference/dependency traversal and texture counting to instance materials/lightmaps. Keep raw document IO and existing graphics-free reference parsing capabilities.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No new controls required; legacy scene opening must remain recoverable.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 06-save-coordinator.md
