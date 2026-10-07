# Lightmap baking and UV2 editor workflow

The lightmap baker needs a usable UV2 atlas. The integrated baker runs in an
existing engine render loop and produces one RGB indirect-light multiplier for
each target Drawable. This page describes the UV2 generation and preview flow
shared by `model_edit` and `bg2e_composer`.

## Editor workflow

1. Select the target Drawable (the active model in `model_edit`; one or more
   directly attached Drawables in Composer).
2. Choose the bake resolution and chart padding, then request **Generate UV2**.
3. The editor queues generation and reload through
   [`app::Uv2SafeReload`](api/app/Uv2SafeReload.md). It runs from
   `MainLoop::safeUpdateScene` after `device().waitIdle()` and before the next
   scene traversal. The CPU-only modifier packs every submesh of one Drawable
   into one atlas; selecting multiple Composer targets creates one independent
   atlas per Drawable.
4. Inspect UV1 or UV2 in the embedded [`ui::UvMapPreview`](api/ui/UvMapPreview.md).
   The preview displays a validation result as well as the layout. UV1 is
   preserved during generation.
5. Bake the selected model/targets. The integrated context prepares its
   context-owned ray-tracing scene once per frame; each target baker records one
   accumulation update in the application's render delegate. `model_edit`
   exposes RTAO; Composer supports RTAO and RTGI.
6. After successful generation, the editors mark the document dirty. The
   modified model can be saved through the editor's existing asset-saving
   workflow.

Regenerating UV2 cancels active bake work because changing the atlas invalidates
its accumulation history. The app helper's token cancels queued regeneration
if its owner is destroyed or the scene is changed. The `geo` modifier itself
never performs GPU work. GPU reload belongs to the editor safe-update path.

## Safe reload example

An editor retains the cancellation token while the queued operation is pending.
The helper calls `MainLoop::safeUpdateScene` internally and performs the CPU
modifier and Drawable reload at that safe point:

```cpp
std::shared_ptr<app::SafeUpdateToken> uv2Token;

geo::Uv2AtlasOptions options;
options.resolution = 512;
options.paddingPixels = 4;
uv2Token = app::Uv2SafeReload::regenerate(
    targetNode,
    options,
    [this](const app::Uv2RegenerationResult& result) {
        if (result.success)
        {
            // Refresh the UV preview and mark the owning document dirty.
        }
        else
        {
            // Display result.message in the editor.
        }
    });

// On window cleanup or scene replacement:
uv2Token.reset();
```

The lower-level scheduling shape is shown below. It is useful when implementing
a custom editor workflow; production editor code should prefer
`Uv2SafeReload::regenerate`, which adds target/root checks, completion reporting,
and cancellation handling:

```cpp
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>
#include <bg2e/scene/Node.hpp>
#include <exception>

std::weak_ptr<scene::Node> weakTarget = targetNode;
auto token = std::make_shared<app::SafeUpdateToken>();
app::MainLoop::current()->safeUpdateScene(
    [weakTarget, options]() {
        auto node = weakTarget.lock();
        if (!node)
        {
            return;
        }
        auto* component = node->drawable();
        auto drawable = component != nullptr ? component->drawable() : nullptr;
        if (!drawable || !drawable->isLoaded())
        {
            return;
        }

        try
        {
            geo::GenerateUv2AtlasModifier modifier(drawable->mesh().get(), options);
            modifier.apply();
            drawable->reload();
        }
        catch (const std::exception&)
        {
            // Report the failure through the owning editor's error UI.
        }
    },
    token);
```

In production code, cancel the active bake before scheduling regeneration, as
the two editor windows do. Keep the token alive until completion; destroying it
discards the queued lambda. The callback is run on the main thread after the
safe update and may refresh the preview and editor document state. The helper
requires a running `MainLoop`; a standalone tool instead runs the CPU modifier
before loading the mesh for rendering.

## What an atlas operation preserves

`geo::GenerateUv2AtlasModifier` submits all source submeshes to one xatlas
Atlas and requires exactly one output atlas. It reconstructs mesh arrays
transactionally, retaining submesh ordering, triangle counts, material
association, and all source attributes other than UV2. In particular, UV1 is
copied bit-for-bit through xatlas's source-vertex mapping. A failed CPU
`apply()` leaves the original mesh untouched.

xatlas is MIT-licensed, copyright 2018–2020 Jonathan Young. The required
copyright and permission notice is distributed in
[`lib/third_party/xatlas/LICENSE.txt`](../lib/third_party/xatlas/LICENSE.txt).

The public validator does not infer UV2 provenance. A copied UV1 channel is
accepted if it already passes the usable-atlas rules. Use
[`geo::UvAtlasValidator`](api/geo/UvAtlasValidator.md) to check finite
in-range coordinates, triangle area, overlaps, indices, and submesh ranges.

The baker dilates each island into its empty UV gutter on the GPU after every
accumulation update. Both `image()` and `readPixels()` use the dilated result.
`LightmapSettings::dilationPixels` controls the radius (default 4); both editor bake windows expose it as
**Lightmap Dilation**. This color fill is separate from **UV2 Padding**, which
only controls the spacing between generated UV islands.

## Integrated API

The engine API is documented in
[`render::IntegratedBakerContext` and `LightmapBaker`](api/render/LightmapBaker.md).
It requires an active frame lifecycle and a valid UV2 atlas. See also the
[geometry API](api/geo/index.md), [render API](api/render/index.md),
[UI preview](api/ui/UvMapPreview.md), and
[safe scene updates](safe_update_scene.md).

The next implementation phase adds standalone, headless CLI baking and will
document its command-line options and output files separately. That future
CLI will use the same CPU modifier before loading meshes, without requiring a
`MainLoop`.
