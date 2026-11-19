# Step 09: Protect resident resources during scene cleanup

## Prerequisite

Step 08 implemented and buildable; record its user validation status.

## Work

Connect runtime protection roots to SceneResourceManager without making its existing file-only mode require a device. Protect cached models and recursive dependencies, independent instance material/lightmap references and required retained resources. Include cache generation in cleanup review/revalidation; loading a resource invalidates an old deletion plan.
IMPORTANT: current Composer cleanup saves and transactionally reloads the saved scene before deletion. Distinguish this internal reload/rebind from user New/Open. Preserve cache-only resident entries and their dependency protection through internal cleanup reload, with explicit ownership/context transfer; user New/Open still starts fresh. Do not clear cached resources as an incidental consequence of cleanup.
Keep runtime roots out of scene persistence. After a true reopen, previously unused nonpersisted cache entries no longer protect files. Preserve existing warning acknowledgement, stale-plan rejection and confirmation semantics. Do not conflate DrawableRegistry, resident cache and disk inventory; reuse compatible bookkeeping only after tracing actual registry call sites.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

User validates unused cached BG2/textures cannot be deleted, cache load invalidates analysis, cleanup internal reload preserves residency, and true reopen removes unused-cache protection.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 10-bake-offscreen.md
