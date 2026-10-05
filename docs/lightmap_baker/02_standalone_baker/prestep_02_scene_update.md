# Handoff for Drive scene updates and own the TLAS

Next implementation step: [step-02_scene_update.md](step-02_scene_update.md). Complete this file **after** finishing step 01; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-02_scene_update.md.

## Next-step instructions

Implement headless scene/component initialization and update, transform/light/environment collection, owned frame/descriptor resources and RayTracingScene::update recording. Support explicit updateScene before any number of target bakes. Define synchronization before ray use and cleanup on errors. Verify behavior for scene components that modify transforms or scene contents.

## Next-step acceptance gate

A headless fixture can prepare a complete scene and valid TLAS without a windowed render loop. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
