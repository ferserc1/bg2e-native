# Step 06: Coordinate model export independently of instances

## Prerequisite

Step 05 implemented and buildable; record its user validation status.

## Work

Refactor complete resource saving in db/draw_resource_io.cpp around an operation-local resource/export table. Instance serialization writes JSON only; it never invokes BG2 writing or mutates original material definitions.
Export each referenced shared resource once. With a DrawableComponent owner, use owner materials/properties; with instances only, use immutable original definitions retained by the entry. Do not export unused cached entries merely because resident. Handle procedural resources, copied owners, material textures, shared paths and Save As transactionally. Keep reference-only saves separate from complete export.
Normal saving MUST overwrite the intended existing scene/model files to persist edits. Only collision-copy destinations require unused paths. Stage publication consistently with current transactional IO; preserve previous good files and pending changes on recoverable failures. Continue independent save work, aggregate diagnostics and never report complete success or publish dangling references after an incomplete export. If the root destination itself cannot be written, return controlled failure with the prior document preserved; no crash or uncaught exception.
Design the table so a future top-level drawables JSON array can persist entries independently of the tree, but do not emit that array now. Identity must survive owner absence and must not depend on tree position.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No dedicated controls yet. Existing save/Save As must preserve owner edits and migrate duplicate paths.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 07-composer-browser.md
