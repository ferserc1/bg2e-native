# Handoff for Plan safe editor mesh replacement

Next implementation step: [step-05_safe_reload.md](step-05_safe_reload.md). Complete this file **after** finishing step 04; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-05_safe_reload.md.

## Next-step instructions

Add or reuse a narrow application-facing helper for invoking the CPU modifier and Drawable::reload inside MainLoop::safeUpdateScene. The callback runs after Engine::device().waitIdle(); use SafeUpdateToken to cancel work after a scene/window swap. Keep the geo API CPU-only and no automatic reload in it. Preserve material/submesh state and invalidate relevant bake histories after reload.

## Next-step acceptance gate

Loaded meshes can receive UV2 updates without modifying GPU resources in flight. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
