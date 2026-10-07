# Step 07: Integrate into model_edit and bg2e_composer

## Scope

In model_edit, add a settings window and bake action for the active model, fixed to RTAO. In bg2e_composer, add a window listing Drawable nodes, target selection, RTAO/RTGI, resolution, accumulation frames and relevant sample/bounce/distance settings. Reuse one integrated context for multiple selected nodes. Save generated images to temporary paths and assign the same image as AO on every submesh material with UV set 1. Rely on existing save-time asset copying. Keep UI responsive according to the existing synchronous pattern; modal progress/blocking is out of scope.

## Acceptance and compile gate

Both applications compile and can trigger a bake using pre-existing UV2, with AO textures attached to all target submesh materials. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `ModelLightmapWindow` to model_edit and `SceneLightmapWindow` to bg2e_composer, using only `bg2e::ui` public wrappers. Register each in its AppDelegate/toolbar. model_edit targets its active model only, fixes RTAO, and exposes resolution, frames and samples. RTAO uses a fixed 0.1 m occlusion radius.
- model_edit is a model-asset editor, so its AO bake must see the target model without the stage's preview floor or other editor-only geometry. Mark the floor Drawable `rayTracingEnabled(false)` during stage setup and exclude any similar preview fixture before building the context-owned bake TLAS. Do not alter the target mesh or replace the active render loop.
- Composer lists every node that directly owns a standard Drawable, independently of current selection. Support multi-selection, one shared integrated context, one baker and one image per selected node. Expose RTAO/RTGI, resolution, frames, samples, GI bounces and RTGI max distance; disable irrelevant fields for RTAO.
- UI actions enqueue bake requests. After the loop's normal component update, each render delegate calls `context->prepareFrame(cmd, frameResources)` once, then records one `baker->update(cmd, frameResources)` per selected target. The baker's TLAS is independent of the renderer's TLAS and of its `prepareSceneRender` order. After the final recorded frame has been submitted and the engine has advanced, read RGB8 outside command recording, write a temporary PNG, create one filesystem `base::Texture`, and set it as `aoTexture` with `aoUVSet=1` on every target submesh material. Call the existing material update path safely. Mark each document dirty. On scene swap, cancel queued requests and release context-owned RT scenes and baker resources after GPU completion.
- The temporary file must live long enough for Save/Save As; rely on current db copying behavior and do not add copy-verification work. GUI progress/modal blocking is not part of this phase. Reject targets with invalid UV2 with a visible message.
- Verify that two Composer targets produce two files but share one context; a multi-submesh target references the same AO path on every material. model_edit never offers GI controls.

## UI review example

Composer list row: `[ ] Sofa/Arm`; settings: `Mode RTGI | 512 | 32 frames`; action: `Generate selected`.

## Handoff

After this step, complete [prestep_08_documentation.md](prestep_08_documentation.md) for the next step (Document integrated baking).
