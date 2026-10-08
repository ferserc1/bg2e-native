# Rendering API

The `bg2e::render` namespace is the production Vulkan rendering API. Its
umbrella header is `<bg2e/render/all.hpp>`. Rendering and baker resources are
owned by an `Engine` and its active frame lifecycle.

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
