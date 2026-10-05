# Handoff for Reuse frame RT data safely

Next implementation step: [step-03_scene_bindings.md](step-03_scene_bindings.md). Complete this file **after** finishing step 02; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-03_scene_bindings.md.

## Next-step instructions

Connect IntegratedBakerContext to the current frame's production RayTracingScene and object/material/light bindings. Expose narrowly scoped accessors or extract shared setup where required; preserve existing RendererDeferred and DeferredLayer APIs. Require invocation after TLAS update, include GPU synchronization, and never store borrowed frame handles past update. Validate the TLAS belongs to the supplied scene root.

## Next-step acceptance gate

Integrated context can inspect a prepared frame and reject missing or mismatched RT state; existing rendering still compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
