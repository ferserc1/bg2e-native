# Architecture

- [Command/memento editing](command-memento/index.md): reusable app history API,
  optional SDK sessions, and Composer's retained scene/resource integration.

- [Lightmaps](lightmaps/index.md): UV atlas construction, surface projection, lighting-pass integration, accumulation, execution contexts, previews and output/material limits.
- [MainLoop, render, draw and gpu](MainLoop_render_draw_gpu.md): application lifecycle and graphics framework ownership.

- [Draw advanced lighting](draw_advanced_lighting.md): native capability split, public surfaces/signals, histories/filtering and ownership.

- [Integrated draw/bake lightmaps](draw_integrated_lightmaps.md): sample/result
  lifecycle, UV2 replacement and explicit RGB indirect material adoption.

- [Headless draw and camera accumulation](draw_offscreen_rendering.md): context
  ownership, exact completed samples, final-only conversion and PNG policy.


- [Standalone UV batch ownership and publication](draw_standalone_lightmaps.md):
  private assembly, frozen serial jobs, typed maps, complete models and final manifest.

- [Production build/distribution](build_and_distribution.md)
- [Resource reuse and profiling](production_performance.md)
