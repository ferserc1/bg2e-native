# bg2 engine documentation

Draw is the production scene renderer over Vulkan or Metal. Start with the
[API index](api/index.md), [draw tutorials](tutorials/draw/index.md) and
[applications](apps/index.md). GPU tutorials cover lower-level resources.

- [Architecture](architecture/index.md) and [window configuration](window_api.md)
- [Migration and breaking changes](migration_to_draw.md)
- [Build, native shaders and distribution](architecture/build_and_distribution.md)
- [Deferred rendering](deferred_render_model.md) and [application delegates](delegate_system.md)
- [Lightmaps](lightmap_baking.md) and [standalone baking](tutorials/draw/standalone_lightmaps.md)
- [Safe scene updates](safe_update_scene.md), [workspace](workspace_api.md), [input](input_delegate.md)
- [Resource reuse and profiling](architecture/production_performance.md)

API availability and native/backend capability are separate. Documentation snippets
are source-based; native build/runtime/package validation belongs to the project lead.
