# Drawable instances implementation plan

## Status and objective

Steps 01–10 implemented and accepted. The user confirmed independent instance lightmaps, validated the owner-appearance creation correction and stated that everything is correct before authorizing documentation. Step 11 documentation is delivered for final review. Next step: None; no implementation or documentation work remains in this plan beyond user review. Reduce resident CPU/GPU geometry and BLAS duplication while keeping independent instance appearance and submesh transforms. Draw-call reduction, HTTP server implementation and cache persistence are out of scope.

## Agreed contracts

- DrawableComponent, DrawableInstanceComponent and TextComponent share DrawableRenderComponent, not each other. Keep Node::drawable() concrete and add Node::drawableInstance().
- Instances cannot mutate geometry. Copy original properties on creation; serialize their ordered submesh state in component JSON. Match by index, truncate excess, fill missing values from originals and warn without cancelling higher-level operations.
- One owner per resource; many instances. Owner replacement propagates shared geometry. Owner duplication/legacy duplicate paths produce separate identities and BG2 files.
- Share local-space raster buffers and BLAS; world transforms belong to TLAS instances. Preserve GPU lifetimes and debug identifiers.
- Device-backed cache is independent of UI/tree, strongly resident, and distinct from disk resource accounting. Do not add a device-free abstraction. BG2 IDs use full paths; procedural/imported IDs are unique (imported IDs have filename prefixes).
- Instances never export or alter models. A DB save coordinator writes each used resource once, using owner properties or original definitions if ownerless. Normal saves overwrite intended files. Collision copies do not overwrite unrelated files.
- Recoverable/palliable failures warn and continue composed operations. Direct invalid assignments fail locally with a UI explanation. Preserve authored state and prior good files; report incomplete saves honestly. No crashes or uncaught recoverable exceptions.
- Creating from a selected owner copies its current in-memory appearance independently, including unsaved textures/lightmaps and submesh properties; creating from the cache uses immutable originals. Both share the same geometry/BLAS identity.
- Cache load/unload from the browser is outside history; scene edits are undoable. Protect cache-only dependencies from cleanup. True New/Open clears residency; internal cleanup reload must preserve runtime protection.
- Gizmo: two small offset wireframe cubes, hidden by default. Per-instance lightmaps live in instance materials.
- Keep identity/export planning ready for a future top-level drawables array; do not persist unused cache entries now.

## Execution and build policy

Each numbered step is a complete, buildable change: finish declarations, implementations and call-site adaptations together. Do not leave deliberately broken intermediate steps. Implementation was subsequently authorized step by step by the user.

After implementation starts, inspect AGENTS.md and the architecture reference before engine changes (the requested skills.md was not found by the planning inventory; locate the current equivalent). Do not modify CMake files. Follow repository GPL, brace, GLM and UI-wrapper rules. Do not alter unrelated APIs or bg2e::render/gpu internals.

The user requires a compiling graphics engine after every implementation step. Use the existing correctly configured build tree and platform generator; do not compile during plan authoring. At implementation time, run the appropriate existing build as the per-step check, recording the exact command/result. Do not guess a Vulkan SDK path or reconfigure with a different generator. If a build cannot run, report it as unverified rather than passed. Build relevant Composer targets once touched. Documentation-only final work does not require recompilation unless source changes occur.

The user accepted step 01 after their own compilation and Composer checks, reported installing Metal Toolchain, and asked for no additional agent testing in the step 02 continuation. No agent builds/tests were run for steps 02–10 under that standing instruction. The user subsequently confirmed successful compilation through step 03 and authorized step 04. Record that user result separately from agent checks; the user explicitly marked step 04 complete after its declaration correction. The user confirmed successful compilation of step 05 and authorized step 06. The user confirmed successful compilation of step 06 and authorized step 07. The user subsequently accepted step 07 and authorized step 08. The user subsequently confirmed successful step 08 checks and explicitly marked that step complete. The user confirmed successful cache and scene cleanup and explicitly accepted step 09. The user subsequently confirmed the step 10 lightmap behavior and owner-appearance correction, accepted the final implementation and authorized step 11. This records user acceptance, not a new agent build or an inferred backend/package test matrix.

Keep optional probes minimal and simple; no large automated test initiative or benchmarks. User-led Composer validation is authoritative. Later steps expose real features incrementally. Do not claim manual checks passed without user confirmation.

## Continuation protocol

1. Read this file and progress.md, inspect the worktree, then read the file for Next step. Preserve unrelated changes.
2. Implement only the selected step and necessary build-preserving integration. Consult prerequisites and actual source; proposed API spellings are design guidance, not preexisting APIs.
3. Build and record minimal checks. Update progress.md with changes, touched areas, build command/result, recovery limitations, Composer checks, user feedback, and the exact next step/file.
4. Report in Spanish: what changed, whether/what the user should test in Composer, and the next step. Stop at the step boundary; do not silently implement the whole plan.
5. Record validation as pending until confirmed. Incorporate requested fixes before advancing. All implementation steps must be accepted by the user before step 11 documentation.
6. If interrupted, keep Next step on the unfinished step and record remaining work; do not mark partial work complete.

## Steps

1. [Introduce the common renderable contract](01-renderable-base.md)
2. [Implement the device-backed drawable cache and resource identity](02-cache.md)
3. [Implement instance state and integrate rendering](03-instance-component.md)
4. [Integrate exclusive owners, replacement and scene lifecycle](04-ownership-lifecycle.md)
5. [Serialize instance properties and recover legacy duplicates](05-scene-format.md)
6. [Coordinate model export independently of instances](06-save-coordinator.md)
7. [Expose the cache browser and basic instance creation](07-composer-browser.md)
8. [Complete instance editing, replacement and the small gizmo](08-composer-editing-gizmo.md)
9. [Protect resident resources during scene cleanup](09-cleanup.md)
10. [Finish bake, offscreen and composed-operation recovery](10-bake-offscreen.md)
11. [Update documentation after user acceptance](11-documentation.md)

## Existing documentation findings

SceneResourceManager.md documents file-only dependency analysis, reviewed snapshots, stale-plan checks and Composer's save/reload-before-cleanup flow. That last flow needs explicit cache preservation; simply clearing on every reload would invalidate the feature.

SceneReferences.md documents reference-only transactional saves, raw JSON IO, component recovery boundaries and Text special cases. DrawResourceIO.md documents complete engine-aware export, texture packaging and transactional publication. These documents require coordinated updates in the final step, along with command-memento resource retention and lightmap material/output descriptions.
