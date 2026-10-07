# Handoff for Document UV2 generation and previews

Next implementation step: [step-07_documentation.md](step-07_documentation.md). Complete this file **after** finishing step 06; this template records no implementation results.

- Changed files and relevant API decisions: integrated UV2 generation + preview into both editors. `apps/model_edit/src/ModelLightmapWindow.{hpp,cpp}`: new "UV Atlas" section with padding slider, "Generate UV2" button and embedded `ui::UvMapPreview` fed from `stage()->targetDrawable()->mesh()`; `generateUv2()` cancels any active bake, calls `app::Uv2SafeReload::regenerate(stage()->targetModelNode(), {resolution = 128<<_resolutionIndex, paddingPixels}, callback)`; on success marks `document()->setUnsavedChanges(true)` and refreshes the preview. `apps/bg2e_composer/src/SceneLightmapWindow.{hpp,cpp}`: same section for the multi-selection — one independent `regenerate` per selected node (one atlas per Drawable covering all its submeshes), tokens stored in a vector, per-node completion counts, document marked dirty per success; the preview shows the first selected target's mesh. Both windows destroy their tokens in `cleanup()` to cancel pending work. No new engine API in this step; step-05 helper and step-04 widget were used unchanged.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification of step 06 remains outstanding.
- Runtime/fixture evidence: none; no tests in step 06. Manual end-to-end verification (UV1 equality, island layout, shared two-submesh AO texture, bake result) is pending per the step-06 deliverables.
- Remaining limitations or regressions: in Composer the UV preview shows only the first selected target. The "Generate UV2" buttons block while regenerations are pending but the actual work runs once inside the next `safeUpdateScene` drain (blocking frame after `waitIdle`); UI progress indication remains deferred per plan scope.
- Resources, ownership and synchronization cautions for the next agent: the `app::Uv2SafeReload` usage pattern shown by both windows (token storage, cancellation in cleanup, bake cancellation before regeneration, dirty-marking in the callback) is the reference example the documentation must include. Public UI signatures to document: `ui::UvMapPreview::{init,setMesh,setUvSet,setResolution,setDisplaySize,refresh,draw,cleanup}`, `app::Uv2SafeReload::regenerate`, `app::Uv2RegenerationResult` fields, `geo::GenerateUv2AtlasModifier` (`apply()`, `result()`, `Uv2AtlasOptions{resolution,paddingPixels}`, `Uv2AtlasResult{width,height,chartCount,utilization}`), `geo::UvAtlasValidator::validate(const geo::Mesh&, uint32_t)` and the `UvAtlasValidation`/`UvAtlasError` fields, and `render::UvMapPreviewRenderer::{UvMapPreviewRenderer(engine,resolution),setResolution,resolution,render(mesh,uvSet),texture}`.
- Exact next action: Implement the scope in step-07_documentation.md.

## Next-step instructions

Update doc/api/geo, doc/api/render and doc/api/ui for the implemented atlas and editor preview. Cover MIT license, UV1 preservation, shared atlas across submeshes, CPU-only modifier and safe editor reload. The phase-3 CLI documents its own flag and outputs after implementation.

## Next-step acceptance gate

Documentation matches the completed behavior and links between namespaces. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/geo/GenerateUv2AtlasModifier.md` and `UvAtlasValidator.md` with CPU-only behavior, xatlas MIT attribution, transactional failure, UV1 preservation and one-atlas/multiple-submesh semantics. Update geo index/reference.
- Add `doc/api/render/UvMapPreviewRenderer.md` and `doc/api/ui/UvMapPreview.md`; show both UV channels, image lifetime and the rule forbidding ImGui headers outside the engine UI implementation. Update namespace indexes.
- Update `doc/api/render/LightmapBaker.md` and `doc/lightmap_baking.md` with the integrated editor flow. Include the safe `MainLoop::safeUpdateScene` reload example. State that phase 3 will add headless CLI use of the same modifier.
- Check every code example against actual signatures and all Markdown links. CLI output cases are documented in phase 3.
