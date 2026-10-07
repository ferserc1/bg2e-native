# Step 08: Document integrated baking

## Scope

Update doc/api/render with context, baker, settings, result and lifetime contracts. Add doc/api/ui references for both windows, a technical description in doc/ when useful, and UV2 fixture instructions. Describe RGB8/float output, AO-slot meaning, `prepareFrame`/baker frame ordering, context-owned TLAS synchronization, and no scene lifecycle calls in integrated mode.

## Acceptance and compile gate

Documentation links resolve and match the implemented public API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `doc/api/render/LightmapBaker.md` with the exact public types, settings validation, ownership, frame-order sample, UV-space `GBufferManager` profile and RGB result semantics from [API_CONTRACT.md](../API_CONTRACT.md). Add `doc/api/render/GBufferManager.md` for its default and depthless configurations, then link both pages from the render API index/reference.
- Document `ModelLightmapWindow` and `SceneLightmapWindow` under `doc/api/ui/`, including temporary output and all-submesh AO assignment. Update the UI index/reference. Add a technical `doc/lightmap_baking.md` linking to the API page and explaining the UV-space pass, the RTAO/RTGI modes, accumulation and context-owned TLAS lifetime across in-flight frames.
- State that integrated mode invokes no component lifecycle calls, needs a pre-existing usable UV2, uses full resolution, and does not expose ImGui headers. Reconcile source examples with actual final signatures, not draft pseudocode.

## Review checklist

Every documented method and link must resolve to implemented source; remove stale statements from `doc/deferred_render_model.md` where its AO/GI material behavior changed.

## Handoff

After this step, complete [prestep_01_lifecycle_contract.md](prestep_01_lifecycle_contract.md) for the next step (Define standalone lifecycle).
