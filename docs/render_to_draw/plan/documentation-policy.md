# Cumulative documentation at milestone closure

[Plan index](index.md) · [Execution protocol](execution-protocol.md) · [Review protocol](review-protocol.md) · [Progress](progress.md)

## User-directed change

M01–M09 each end with a separate documentation implementation step after the technical code-versus-plan review. Both are required tracked steps with normal lead acceptance. M10 remains, as a smaller final audit of the documentation already produced. This changes scheduling, not the final documentation requirements. Adding these planning files does not implement the documentation steps.

## Cumulative scope and truthful availability

At closure of milestone N, maintained `doc/` describes the actual implementation through N, including corrections to earlier milestones. Update the same canonical pages and navigation; do not create ten competing API snapshots. Reference declarations, source, registered settings and delivered example/app callers, not proposed names. Do not write usage snippets, references or tutorials for unimplemented milestones. Pending work belongs in the development plan and coverage ledger, not in an API guide claiming future functionality exists.

Through M08, retain truthful render/draw coexistence and production status and accurate legacy guidance. M09 updates the full maintained set to canonical draw production, removing obsolete working-code guidance and experimental-opt-in claims. Historical analysis/planning under `docs/render_to_draw/` remains historical evidence; preserve chronology and distinguish it from maintained user documentation. Accurate foundation pages may stay unchanged, but their examples/dependencies/cross-links must be checked when affected.

## Preserved final coverage and authoring ownership

| Former M10 work | Distributed implementation owners | Final audit |
|---|---|---|
| Every maintained document inventoried; keep/update/remove/replace disposition and source mapping | Each M01–M09 documentation step updates affected-page inventory; M09 completes repository-wide reconciliation, including retained foundation pages | M10-01, M10-05 |
| API indexes/reference, lifecycle/ownership and namespace boundaries across draw/pipeline/environment, gpu, reflection, scene, bake, utils, manipulation, UI, DB and app | M01 foundations/deferred/settings/DB/UI; M02 manipulation/utils; M03 effects/histories; M04 bake; M05 offscreen/app; M06–M08 complete IO/apps/CLI; M09 canonical API and retirement | M10-02 |
| Live accessor/reflection catalogs, settings identifiers/schema/precedence/import/export, renderer/upscaler rebinding, deep CPU/GPU clone and safe rare edits | M01, updated by M02–M09 when their actual contracts change | M10-02–M10-04 |
| Primitive capabilities versus runtime SDK/effect support, explicit Vulkan/Metal indices/native boundaries, query versus RT kernels, compatible object/subclass/shader injection, external renderer/upscaler hooks | M01 and M03, maintained by subsequent owning milestones | M10-02–M10-03 |
| Runnable tutorials for deferred/query shadows/default scale/FSR, manipulation/MaterialModifier, effects/histories, integrated lightmaps and accumulated offscreen output | M01, M02, M03, M04, M05 respectively; later changes update existing tutorials | M10-03 |
| Shader folders, no-RT/SDK-unavailable fallback, UV-map/sample/time/color semantics, clone independence and capability limits | Each delivering milestone, especially M01–M05 | M10-02–M10-03 |
| Supported apps, CLI precedence/output/counts/formats/preflight/errors/overwrite policy, UI/import/viewpoint/bake and async/safe-edit lifecycle | M05 offscreen; M06 model editor; M07 composer; M08 standalone/batch bake | M10-04 |
| Build/platform generators, resources, SDK/native deployment, packages, standalone projects/library updates, final breaking changes and removal of old APIs/forward/MSAA promises | Relevant M01–M08 products maintain current instructions; M09 completes final distribution and retirement reconciliation | M10-04 |
| Navigation/anchors, symbols/header/target/resource paths, snippets, diagrams, obsolete status/namespace/alias references, all retained pages | Every documentation step for affected scope; M09 final complete set | M10-05 |
| Actual-code/product versus completed-plan review, explicit scope limitations and follow-ups | Every technical review plus documentation handoff; M10 closes the complete audit | M10-06 |

No future ResourceManager, native Metal upscaler, DLSS, compatibility shim or unsupported forward/MSAA behavior is invented to fill a documentation gap. Documentation authoring does not authorize new engine features.

## Maintained page families

Keep the original final page families within the distributed inventory: `doc/api/{draw,gpu,reflection,scene,bake,utils,manipulation,ui,db,app}/`, `doc/tutorials/{draw,gpu}/`, `doc/apps/`, and affected conceptual guides including `doc/deferred_render_model.md`, `doc/delegate_system.md`, `doc/lightmap_baking.md`, `doc/workspace_api.md`, `doc/safe_update_scene.md`, `doc/window_api.md` and `doc/input_delegate.md`. Paths are coverage targets, not claims that every folder or future API already exists. Only create/update a guide when its functionality has actually been delivered; preserve valid current foundation/production guides until their actual retirement owner changes them.

## Evidence and completion

During execution, each milestone writes `documentation.md` with changed/retained/retired page dispositions, actual source/example mappings, cumulative coverage, links/snippet checks, validation ownership and honest remaining limits. Append the handoff to the technical `review.md`; this report does not replace updates under `doc/`. Carry remaining future requirements to their actual owner above, without documenting their future implementation.

The agent performs static checks. Builds/runtime demonstrations remain lead-owned unless explicitly authorized; report unverified snippets and unspecified backend coverage honestly. Documentation-only work follows the same one-step delivery/acceptance convention. The technical review points next to its own documentation step; the documentation step points to the following milestone only after recording the normal acceptance condition. Known failures and explicit lead deferrals stay visible.

M10 audits and makes localized corrections to the accumulated documentation. Missing substantive API/tutorial/app coverage is an unfulfilled earlier documentation requirement, recorded against its owner and corrected before completion; it must not become a new deferred authoring phase. Preserve M10 step IDs to keep references stable. M10-06 remains the final plan review/acceptance gate; no redundant authoring milestone is appended.
