# Step 01: Define standalone lifecycle

## Scope

Add StandaloneBakerContext as a distinct public type sharing only the bake core with IntegratedBakerContext. Specify initialize, resize when required, updateScene, bake and cleanup ordering. Define the scene generation/version that invalidates every affected baker history. Do not trigger lifecycle calls from integrated mode.

## Acceptance and compile gate

Headers and implementation stubs compile; invalid call order reports clear errors. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_02_scene_update.md](prestep_02_scene_update.md) for the next step (Drive scene updates and own the TLAS).
