# Lightmap baking, UV2 editors, and standalone CLI

This page documents the existing production `bg2e::render` workflow. The experimental
[draw advanced-lighting API](architecture/draw_advanced_lighting.md) has separate
public components, settings, signals and ownership. The experimental [integrated bake API](api/bake/index.md) and
[lightmap tutorial](tutorials/draw/12_lightmaps.md) now cover UV2 generation,
completed RGB maps and draw material adoption. Standalone draw baking and
windowless application orchestration remain separate pending products. Retain
this production guide for current production editors and CLI applications.

The lightmap baker needs a usable UV2 atlas. The integrated baker runs in an
existing engine render loop; the standalone baker runs without a window or render
loop. Both produce one RGB indirect-light multiplier per target Drawable. This
page describes the UV2 generation, preview, and headless CLI workflows.

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
Bakers select `UvAtlasOverlapPolicy::Warn`: overlaps emit diagnostics and never
abort atlas generation or baking, even if triangles overwrite shared texels.
Other malformed geometry/coordinate errors remain fatal.

The baker dilates each island into its empty UV gutter on the GPU after every
accumulation update. Both `image()` and `readPixels()` use the dilated result.
`LightmapSettings::dilationPixels` controls the radius (default 4); both editor bake windows expose it as
**Lightmap Dilation**. This color fill is separate from **UV2 Padding**, which
only controls the spacing between generated UV islands.

## Integrated API

The integrated engine API is documented in
[`render::IntegratedBakerContext` and `LightmapBaker`](api/render/LightmapBaker.md).
It requires an active frame lifecycle and a valid UV2 atlas. See also the
[geometry API](api/geo/index.md), [render API](api/render/index.md),
[UI preview](api/ui/UvMapPreview.md), and
[safe scene updates](safe_update_scene.md).

## Standalone scene assembly and lifecycle

The CLI loads the JSON context scene, then attaches either one `.bg2` model at
the world origin or a complete prefab subtree. Context and target Drawables are
assembled into one scene before `StandaloneBakerContext::updateScene()`. The
standalone context performs component updates, refreshes scene lights and
environment resources, and builds one owned production `RayTracingScene` TLAS.
Context geometry and target Drawables therefore participate together as GI
occluders. A batch calls `updateScene()` once; baking additional targets does
not rebuild the TLAS. Call `updateScene(deltaSeconds)` explicitly when scene
inputs move or change; this advances the scene generation and resets live baker
accumulation histories.

The standalone API needs no camera, window, `MainLoop`, or renderer. Each
`StandaloneLightmapBaker::update()` records and synchronously waits for one
sample, then advances the engine frame counter. Read pixels after the requested
accumulation count. The engine and assembled scene must outlive their contexts
and bakers; destroy the batch and context before calling `Engine::cleanup()`.

## Standalone UV2 and output policy

Both CLI branches assemble targets with GPU loading deferred. With
`--generate-uv2=true`, the batch applies the CPU atlas modifier to each target
and validates it before loading target GPU meshes/BLASes and building the TLAS.
Generation or generated-atlas validation failures skip the affected target
with a warning and allow the rest to continue. With `false`, it validates existing UV2; invalid targets are reported once and
skipped for baking while remaining in the full scene as potential occluders.
The validator accepts a usable UV1 copy in UV2 and never infers provenance from
coordinate equality.

Output is preflighted for the entire eligible batch before baking. Images are
written in the selected PNG, JPEG, BMP, or TGA format. The no-generation branch
writes images only. The generation branch writes an image and a new `.bg2`
copy per target, setting the generated AO image path on all submesh materials
with UV set 1 and unit scale. Output names derive from sanitized stable node
identities. Input aliases and unresolved collisions are fatal; existing outputs
are replaced by default by the CLI, or rejected with `--overwrite=false`.
Input models, prefab JSON, and context JSON are never overwritten or emitted.
The `.bg2` writer may copy referenced material texture sidecars into the output
directory. See the [CLI guide](api/app/LightmapGenerator.md) for all four
model/prefab and UV2 output combinations.

## Detailed implementation documentation

The [lightmap architecture guide](architecture/lightmaps/index.md) explains the
UV2 surface projection, world-space interpolation, coverage mask, frame ownership,
accumulation, dilation and output boundaries directly from C++ and GLSL.
The [application guide](apps/lightmap_generator/index.md) contains the current CLI.

A dedicated RGB illumination channel in the material model remains future work.
Current exports use the scalar AO property; the deferred shader path already
contains RGB sampling through that same texture, but this is not a separate
material channel. See [material integration and limits](architecture/lightmaps/output_and_materials.md).

## Experimental integrated draw workflow

The [integrated architecture](architecture/draw_integrated_lightmaps.md) describes
the delivered draw/bake path. Its exact completed raw updates are averaged before
final composition and chart-safe dilation. Explicit bakedAO/bakedGI material modes
use linear RGB UV2 through the AO slot; a separate scalar AO map cannot coexist
in that slot. The production export workflow above does not export draw GPU-only
results. Follow [tutorial 12](tutorials/draw/12_lightmaps.md) for source construction
and [draw_lightmaps](apps/draw_lightmaps.md) for controls and deployment.
