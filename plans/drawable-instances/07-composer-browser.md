# Step 07: Expose the cache browser and basic instance creation

## Prerequisite

Step 06 implemented and buildable; record its user validation status.

## Work

Add ui::DrawableInstancesWindow through engine UI wrappers only, integrated into Composer menus and stopped IO boundaries. List identity, label, source, owner/instance use, retention and unload availability. Load BG2 into cache, create an instance from a selected entry, and unload unused resources.
Explicit cache load/unload is outside undo history and does not dirty the scene. Creating, deleting or rebinding a scene instance is undoable. Disable unloading when scene use or outstanding retention makes it unsafe; explain the reason. Expose warnings and direct invalid assignments with message boxes; aggregate diagnostics from composed operations.
Provide create-instance-from-owner as a practical entry point, plus inspection of independent materials/transforms. Do not add placeholders or controls whose backend is unfinished.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

User validates load once/reuse, instance creation, shared geometry, independent materials/transforms, save/reopen, unload guards and history behavior.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 08-composer-editing-gizmo.md
