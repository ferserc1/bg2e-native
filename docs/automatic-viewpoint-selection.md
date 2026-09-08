# Automatic viewpoint selection

`bg2e::render::ViewpointAnalyzer` evaluates camera candidates around an already
loaded scene or node subtree. It is designed for generic industrial and
e-commerce visualization; it does not assume an object front or any product
category semantics.

## Rendering path

The analyzer reuses the production Vulkan render path and the existing
`pick_selection` shaders. Each visible submesh receives a temporary 32-bit ID,
encoded in an RGBA8 attachment, while a D32 attachment records depth. Stencil is
not used.

All views are recorded in one immediate command submission. After each 64x64
render (configurable), the ID and depth attachments are copied into a different
region of CPU-visible readback buffers. CPU analysis starts only after the batch
fence has completed.

The input scene is not modified: the analyzer builds camera matrices directly
from the visible scene AABB and stores weak references in `AnalyzedSubmesh` so
`ViewAnalysis::pixelsPerSubmesh` can be mapped back to scene data.

## Sampling

Every sampled dimension uses `ViewpointSampleRange { min, max, samples,
cyclic }`. Non-cyclic ranges include both endpoints. Cyclic ranges exclude the
maximum, so eight samples over `[0, 360)` produce `0, 45, ..., 315` without a
duplicate view.

Defaults cover the full yaw range, use only horizontal/upper views, and target
the AABB center. A product application can opt into a target height of `0.75`
without making that category-specific heuristic an engine rule.

```cpp
bg2e::render::ViewpointAnalyzer analyzer(engine);

auto config = analyzer.config();
config.width = 64;
config.height = 64;
config.yaw = { 0.0f, 360.0f, 12, true };
config.pitch = { 5.0f, 30.0f, 3, false };
config.targetHeight = { 0.65f, 0.8f, 4, false };
analyzer.setConfig(config);

auto samples = analyzer.analyze(scene);
if (const auto * best = bg2e::render::ViewpointAnalyzer::bestSample(samples))
{
    useCamera(best->camera.position, best->camera.target);
}
```

`generateCandidates()` is virtual for custom sampling policies. `score()` is a
protected virtual function; subclasses can combine coverage, clipping, depth
statistics and per-submesh visibility without changing rendering or readback.
The base score rewards visible coverage and penalizes pixels touching the image
border.

The `Engine` must outlive the analyzer, and analyzed drawables must already be
loaded on that engine. Disabled nodes and invisible submeshes are excluded, in
the same way as the scene bounding-box calculation.
