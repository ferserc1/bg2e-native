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
