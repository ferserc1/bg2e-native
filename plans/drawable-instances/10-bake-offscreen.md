# Step 10: Finish bake, offscreen and composed-operation recovery

## Prerequisite

Step 09 implemented and buildable; record its user validation status.

## Work

Audit integrated/standalone bake assembly, LightmapOutputWriter and Composer lightmap commands for base-contract/instance support. Bind per-instance lightmap images through serialized materials; never change originals or other instances. Keep output identity per instance, geometry/BLAS retention per resource.
Verify cache operation with initialized graphics backend and no Composer/tree dependency for offscreen jobs; do not implement an HTTP server, device-free cache or draw-call batching. Add only minimal existing-surface probes if useful, not a new testing framework.
Complete all composed-operation diagnostic aggregation and recovery gaps introduced by this feature. Inspect failed model loads/copies, duplicate paths, incomplete exports, pending jobs, close/new/open and undo retention. Keep recovery scoped to resources that can be preserved or defaulted; do not fabricate successful writes or invalidate GPU lifetime contracts.
Add any remaining Composer options needed to exercise bake/offscreen resource selection incrementally, using existing workflows rather than unrelated tooling.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

User validates different lightmaps on shared geometry, bake/save/reopen, offscreen reuse, memory residency and recoverable failures. Collect final implementation acceptance before documentation.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 11-documentation.md
