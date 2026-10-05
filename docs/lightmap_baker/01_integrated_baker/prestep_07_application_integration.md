# Handoff for Integrate into model_edit and bg2e_composer

Next implementation step: [step-07_application_integration.md](step-07_application_integration.md). Complete this file **after** finishing step 06; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-07_application_integration.md.

## Next-step instructions

In model_edit, add a settings window and bake action for the active model, fixed to RTAO with no RT shadows. In bg2e_composer, add a window listing Drawable nodes, target selection, RTAO/RTGI, shadows, resolution, accumulation frames and relevant sample/bounce/distance settings. Reuse one integrated context for multiple selected nodes. Save generated images to temporary paths and assign the same image as AO on every submesh material with UV set 1. Rely on existing save-time asset copying. Keep UI responsive according to the existing synchronous pattern; modal progress/blocking is out of scope.

## Next-step acceptance gate

Both applications compile and can trigger a bake using pre-existing UV2, with AO textures attached to all target submesh materials. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `ModelLightmapWindow` to model_edit and `SceneLightmapWindow` to bg2e_composer, using only `bg2e::ui` public wrappers. Register each in its AppDelegate/toolbar. model_edit targets its active model only, fixes RTAO and `rtShadows=false`, and exposes resolution, frames, samples and max distance.
- model_edit is a model-asset editor, so its AO bake must see the target model without the stage's preview floor or other editor-only geometry. Mark the floor Drawable `rayTracingEnabled(false)` during stage setup and exclude any similar preview fixture before its normal TLAS build. Do not alter the target mesh or replace the active render loop.
- Composer lists every node that directly owns a standard Drawable, independently of current selection. Support multi-selection, one shared integrated context, one baker and one image per selected node. Expose RTAO/RTGI, RT shadows, resolution, frames, samples, GI bounces and max distance; disable irrelevant fields for RTAO.
- UI actions enqueue bake requests. Render delegates record one update per target per rendered frame **after** their normal TLAS update. Once the last frame completes, read RGB8, write a temporary PNG, create one filesystem `base::Texture`, and set it as `aoTexture` with `aoUVSet=1` on every target submesh material. Call the existing material update path safely. Mark each document dirty. On scene swap, cancel queued requests and release resources after GPU completion.
- The temporary file must live long enough for Save/Save As; rely on current db copying behavior and do not add copy-verification work. GUI progress/modal blocking is not part of this phase. Reject targets with invalid UV2 with a visible message.
- Verify that two Composer targets produce two files but share one context; a multi-submesh target references the same AO path on every material. model_edit never offers GI/shadow controls.
