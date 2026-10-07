# Handoff for Document integrated baking

Next implementation step: [step-08_documentation.md](step-08_documentation.md). Step 07 implementation is present, pending the project lead's build and runtime verification.

- Changed files and relevant API decisions: Added `apps/model_edit/src/ModelLightmapWindow.{hpp,cpp}` (RTAO-only; resolution/frames/samples; progress text; PNG result preview through `ui::TextureWidgets`) and `apps/bg2e_composer/src/SceneLightmapWindow.{hpp,cpp}` (multi-selectable Drawable-node list under the editable root, RTAO/RTGI combo, GI bounces disabled for RTAO). Both AppDelegates gained `requestLightmapBake`/`cancelLightmapBake`, a shared `IntegratedBakerContext` (created lazily, `setEnvironmentResources(renderer()->environmentResources())`), per-target `IntegratedLightmapBaker` jobs, and overridden `render()` (records `prepareFrame` once plus one `baker->update` per target after the normal scene draw) and `update()` (outside command recording, performs `readPixels()` once the engine frame has advanced, writes a unique PNG under `std::filesystem::temp_directory_path()`, then assigns one filesystem `base::Texture` as AO with `aoUVSet=1`, `aoScale={1,1}` on every submesh via `drawable->material(i)` + `renderMaterial(i)->setMaterialAttributes()` + `updateTextures()`, and marks the document dirty). model_edit: floor Drawable is `setRayTracingEnabled(false)` at stage setup and environment restore, `StageScene::close()` cancels the bake, new `StageScene::targetModelNode()` getter. Composer: the existing `onSceneSwap` callback also cancels the bake. Both ToolBars gained a "Lightmaps" button and a Window menu entry. No public engine API changed; no CMake edit was needed (apps glob their sources).
- Build command, platform and result: Not run. AGENTS.md prohibits compilation unless the user explicitly requests it; step-07's compile gate needs the project lead's explicit authorization. `git diff --check` passed.
- Runtime/fixture evidence: None for this step. The step-06 fixture verification confirmed by the project lead remains the last runtime evidence.
- Remaining limitations or regressions: `readPixels()` blocks on device idle, so the readback update stalls one frame (synchronous pattern, no progress UI, per plan). A bake started and then invalidated by a target edit (modifier buttons) is not auto-reset; cancel and re-bake. The composer context is kept across bakes while the scene root is unchanged; it is released on scene swap and app cleanup.
- Resources, ownership and synchronization cautions for the next agent: Baker updates are recorded in the render delegate after the renderer's own draw, never from UI callbacks. Readback only happens in `update()` and retries while the engine frame has not advanced (`std::logic_error`). Preview textures load from unique per-bake temp PNG paths to avoid TextureCache staleness. Bakers/context release GPU resources via the engine's deferred cleanup, so cancellation never frees in-flight resources.
- Exact next action: Ask the project lead to compile both applications and exercise a bake in each, then implement the scope in step-08_documentation.md.

## Next-step instructions

Update doc/api/render with context, baker, settings, result and lifetime contracts. Add doc/api/ui references for both windows, a technical description in doc/ when useful, and UV2 fixture instructions. Describe RGB8/float output, AO-slot meaning, `prepareFrame`/baker frame ordering, context-owned TLAS synchronization, and no scene lifecycle calls in integrated mode.

## Next-step acceptance gate

Documentation links resolve and match the implemented public API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/render/LightmapBaker.md` with the exact public types, settings validation, ownership, frame-order sample, UV-space `GBufferManager` profile and RGB result semantics from [API_CONTRACT.md](../API_CONTRACT.md). Add `doc/api/render/GBufferManager.md` for its default and depthless configurations, then link both pages from the render API index/reference.
- Document `ModelLightmapWindow` and `SceneLightmapWindow` under `doc/api/ui/`, including temporary output and all-submesh AO assignment. Update the UI index/reference. Add a technical `doc/lightmap_baking.md` linking to the API page and explaining the UV-space pass, the RTAO/RTGI modes, accumulation and context-owned TLAS lifetime across in-flight frames.
- State that integrated mode invokes no component lifecycle calls, needs a pre-existing usable UV2, uses full resolution, and does not expose ImGui headers. Reconcile source examples with actual final signatures, not draft pseudocode.
