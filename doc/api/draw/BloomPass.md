# BloomPass and final HDR bloom

Headers: `<bg2e/draw/pipeline/BloomPass.hpp>` and
`<bg2e/draw/pipeline/DeferredLayer.hpp>`. Namespace: `bg2e::draw::pipeline`.
The built-in `FinalLayer` owns a `BloomPass` and public `BloomSettings bloom`.
See [architecture](../../architecture/draw_bloom.md) for filters, blend equations,
memory estimates, backend requirements and validation limits.

## Settings

| BloomSettings member | Type | Default | Supported values / meaning |
|---|---|---|---|
| enabled | bool | true | Enables bloom draws and lazy pyramid allocation |
| intensity | float | 0.08 | 0..10; multiplier applied in final composition; zero skips bloom |
| threshold | float | 1.0 | 0..65504; scene-linear maximum RGB component, before exposure |
| scatter | float | 0.7 | 0..1; coarse-level weight during reconstruction |
| levels | uint32_t | 6 | 1..8; maximum pyramid levels, including the first reduction |
| resolution | BloomResolution | Quarter | Half=2 or Quarter=4; initial divisor per dimension |

`BloomSettings::validate()` throws `std::invalid_argument` for unsupported values,
including nonfinite floats. `record()` also validates settings. All dimensions are
rounded up and clamped to at least one; the pyramid stops early at 1x1. Scatter and
level count affect halo extent; there is no independent radius-in-pixels setting.

For each sanitized source texel, extraction is
`RGB * max(maxRGB - threshold, 0) / max(maxRGB, 1e-6)`.
At the default threshold, RGB <= 1 in every component contributes nothing. A texel
with any component > 1 contributes its excess while retaining color ratios.
Extraction precedes initial block averaging. It is not an emission-only mask or a
luminance cutoff. There is no below-threshold soft transition.

## Built-in final-layer integration

Access the retained final layer at a stopped update boundary, or configure it
before initialization:

```cpp
#include <bg2e/draw/pipeline/DeferredLayer.hpp>

// finalLayer is the retained pipeline::FinalLayer selected for the renderer.
finalLayer->bloom.enabled = true;
finalLayer->bloom.threshold = 1.0f;
finalLayer->bloom.intensity = 0.08f;
finalLayer->bloom.scatter = 0.7f;
finalLayer->bloom.levels = 6;
finalLayer->bloom.resolution = bg2e::draw::pipeline::BloomResolution::Quarter;
finalLayer->bloom.validate();
```

For live UI edits, prefer the queued [settings facade](Settings.md#bloom-settings).
The default `RendererDeferred` creates a compatible final layer automatically;
injected custom final layers are responsible for their own postprocessing.

`recordLinear()` leaves HDR free of bloom. `recordFinal(frame, linearColor)`
extracts bloom from the supplied composed/reconstructed RGBA16F image, then adds
it in the existing final draw before exposure/tone mapping and overlays/UI.
This also applies to externally accumulated HDR. The source is unchanged and no
extra full-resolution composition target is created. Bloom runs with bilinear,
FSR and MetalFX reconstruction; it is separate from post-tone-map `FinalProcessor`
and is not skipped merely because temporal reconstruction succeeded.

`FinalLayer::bloomAvailable()` and `bloomStatus()` report initialization capability.
A caught runtime initialization failure disables the effective bloom while leaving
requested settings and ordinary color conversion intact. The renderer calls
`suppressBloom(bool)` for lighting/temporal diagnostics; final debug modes also
suppress it. Suppression does not overwrite `bloom.enabled`. Applications using
`FinalLayer` independently must supply any needed diagnostic suppression.

## Direct pass contract

| Method | Contract |
|---|---|
| initialize(engine, shaderRoot, slots) | Fresh lifecycle and explicit nonzero slot count; loads native fullscreen/bloom shaders and creates pipelines/sampler |
| record(frame, hdr, settings) | One invocation per acquired, completed slot per frame; returns borrowed bloom image or nullptr when disabled/zero intensity |
| cleanup() | Releases all owned images, descriptors and pipelines; GPU work must already be drained |

The frame must match the initializing Engine and a valid slot. Input is a sampled
2D RGBA16F image on that Device. Call outside active command/rendering scopes.
Keep the input alive through GPU completion, and serialize pass access on the
recording thread. `record()` handles its own image transitions, starts/ends its own
rendering scopes and leaves source/pyramid images shader-readable. It does not
submit or wait. It validates engine/slot/device/format; caller obligations about
completion and lifetime remain explicit.

The returned raw pointer is owned by the pass's slot. Retain the pass and avoid
re-recording/replacing that slot until every GPU reader completes. Disabled bloom,
zero intensity, resize/quality changes and cleanup can invalidate the pointer.
Intensity is applied by the final composition consumer, not baked into this image;
a direct consumer must multiply the returned RGB by `settings.intensity` itself.

Each slot retains one reduced RGBA16F chain and reuses descriptors at stable
settings. Extent/resolution/level changes rebuild only the retired slot. Disabled
or zero-intensity recording releases that slot's chain; other slots are untouched.
A direct initialization failure throws; the graceful fallback belongs to
`FinalLayer`. Partial initialization can be cleaned up at a stopped boundary.

GPU profiling exposes `post.bloom`. Native shaders are
`shaders/src/draw/raster/bloom.frag.{glsl,metal}`; final composition uses the paired
`final.frag` shaders. These shaders must be deployed together with the library.
