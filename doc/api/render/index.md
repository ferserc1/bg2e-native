# Rendering API

The `bg2e::render` namespace is the production Vulkan rendering API. Its
umbrella header is `<bg2e/render/all.hpp>`. Rendering and baker resources are
owned by an `Engine` and its active frame lifecycle.

## Production and experimental frameworks

The terms **production** and **experimental** describe the maturity and scope of
bg2 engine's graphics APIs, not build configurations, deployment environments
or compiler optimization settings.

- **Production: `bg2e::render`.** This is the maintained framework used by the
  engine's applications. It provides Vulkan rendering and integrates with the
  production scene, material and UI components. Use it for applications that
  need those established high-level facilities. Production does not imply that
  an application must run on a server or use a Release build.
- **Experimental: `bg2e::draw`.** This is a high-level framework built on the
  backend-neutral `bg2e::gpu` API. Its windowed path supports Vulkan and Metal,
  retained scene color and separate UI composition. Its high-level scene and
  resource contracts have a narrower scope than render and can evolve as the
  API develops; render components cannot simply be passed to draw. Use it when
  evaluating the multi-backend architecture or building against its GPU-based
  delegate contract. Experimental does not mean the code is only a mock or that
  its window/UI path is unavailable.

Draw is intended to become the successor to render. Both APIs coexist; render
remains supported and is not deprecated. `bg2e::gpu` provides lower-level devices,
queues, commands and resources; it does not supply render's high-level scene
framework. Choosing Vulkan in draw does not select render.

## UV inspection and baking

- [`UvMapPreviewRenderer`](UvMapPreviewRenderer.md) renders a CPU mesh's UV1 or
  UV2 layout to a sampled image.
- [`Integrated Lightmap Baker`](LightmapBaker.md) bakes a usable UV2 atlas
  inside an existing application render loop.
- [`Standalone Lightmap Baking`](StandaloneBakerContext.md) assembles a headless
  scene, shares one TLAS across target bakers, and coordinates cancellable
  model/prefab batches without a render loop.
- [`ui::UvMapPreview`](../ui/UvMapPreview.md) embeds the UV renderer in the
  engine UI and adds atlas validation feedback.
- [`geo::GenerateUv2AtlasModifier`](../geo/GenerateUv2AtlasModifier.md) creates
  the CPU-side UV2 atlas; use [`app::Uv2SafeReload`](../app/Uv2SafeReload.md)
  to update a loaded editor Drawable safely.

UV1 is preserved by atlas generation. See [Lightmap baking](../../lightmap_baking.md)
for the model_edit, Composer, and standalone CLI workflows.
