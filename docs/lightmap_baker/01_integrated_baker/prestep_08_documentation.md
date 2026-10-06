# Handoff for Document integrated baking

Next implementation step: [step-08_documentation.md](step-08_documentation.md). Complete this file **after** finishing step 07; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-08_documentation.md.

## Next-step instructions

Update doc/api/render with context, baker, settings, result and lifetime contracts. Add doc/api/ui references for both windows, a technical description in doc/ when useful, and UV2 fixture instructions. Describe RGB8/float output, AO-slot meaning, `prepareFrame`/baker frame ordering, context-owned TLAS synchronization, and no scene lifecycle calls in integrated mode.

## Next-step acceptance gate

Documentation links resolve and match the implemented public API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/render/LightmapBaker.md` with the exact public types, settings validation, ownership, frame-order sample, UV-space `GBufferManager` profile and RGB result semantics from [API_CONTRACT.md](../API_CONTRACT.md). Add `doc/api/render/GBufferManager.md` for its default and depthless configurations, then link both pages from the render API index/reference.
- Document `ModelLightmapWindow` and `SceneLightmapWindow` under `doc/api/ui/`, including temporary output and all-submesh AO assignment. Update the UI index/reference. Add a technical `doc/lightmap_baking.md` linking to the API page and explaining the UV-space pass, four mode/shadow combinations, accumulation and context-owned TLAS lifetime across in-flight frames.
- State that integrated mode invokes no component lifecycle calls, needs a pre-existing usable UV2, uses full resolution, and does not expose ImGui headers. Reconcile source examples with actual final signatures, not draft pseudocode.
