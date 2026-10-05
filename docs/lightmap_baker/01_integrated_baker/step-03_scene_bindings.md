# Step 03: Reuse frame RT data safely

## Scope

Connect IntegratedBakerContext to the current frame's production RayTracingScene and object/material/light bindings. Expose narrowly scoped accessors or extract shared setup where required; preserve existing RendererDeferred and DeferredLayer APIs. Require invocation after TLAS update, include GPU synchronization, and never store borrowed frame handles past update. Validate the TLAS belongs to the supplied scene root.

## Acceptance and compile gate

Integrated context can inspect a prepared frame and reject missing or mismatched RT state; existing rendering still compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_04_rtao.md](prestep_04_rtao.md) for the next step (Implement RTAO baking).
