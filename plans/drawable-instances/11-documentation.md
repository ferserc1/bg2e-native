# Step 11: Update documentation after user acceptance

## Prerequisite

Step 10 implemented and buildable; record its user validation status.

## Work

Only start after the user has validated all implementation steps. Update doc/ to describe actual shipped APIs and behavior, not proposed signatures.
Create doc/architecture/drawable_caches_and_instances.md covering ownership, independent state, identities, shared buffers/BLAS, lifecycle, jobs/history retention, serialization/export coordinator, migration, cleanup protection and future cache persistence boundaries. Link it from architecture indexes.
Update doc/api/db/SceneResourceManager.md (runtime protection, cache generation and cleanup reload policy), SceneReferences.md (component arrays and recoverable loading), DrawResourceIO.md (one export per resource, ownership and overwrites), plus related draw/scene/UI APIs, Composer guide, command-memento retention/geometry and lightmap documentation. Add new component/cache/window API pages and fix Text hierarchy/accessor references.
Review existing resource-management documentation and linked manual checks against final behavior. Do not change unrelated failure contracts merely to make prose uniform. Record user validation and final completion in progress.md.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No new behavior. User reviews documentation and final accepted feature coverage.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: None; plan complete after documentation review.
