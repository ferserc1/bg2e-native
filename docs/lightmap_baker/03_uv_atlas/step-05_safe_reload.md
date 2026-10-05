# Step 05: Plan safe editor mesh replacement

## Scope

Add or reuse a narrow application-facing helper for invoking the CPU modifier and Drawable::reload inside MainLoop::safeUpdateScene. The callback runs after Engine::device().waitIdle(); use SafeUpdateToken to cancel work after a scene/window swap. Keep the geo API CPU-only and no automatic reload in it. Preserve material/submesh state and invalidate relevant bake histories after reload.

## Acceptance and compile gate

Loaded meshes can receive UV2 updates without modifying GPU resources in flight. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_06_application_integration.md](prestep_06_application_integration.md) for the next step (Integrate UV2 generation and previews).
