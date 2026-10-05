# Step 02: Drive scene updates and own the TLAS

## Scope

Implement headless scene/component initialization and update, transform/light/environment collection, owned frame/descriptor resources and RayTracingScene::update recording. Support explicit updateScene before any number of target bakes. Define synchronization before ray use and cleanup on errors. Verify behavior for scene components that modify transforms or scene contents.

## Acceptance and compile gate

A headless fixture can prepare a complete scene and valid TLAS without a windowed render loop. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_03_input_assembly.md](prestep_03_input_assembly.md) for the next step (Assemble model and prefab scenes).
