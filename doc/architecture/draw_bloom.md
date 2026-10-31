# HDR bloom in the draw renderer

The built-in `pipeline::FinalLayer` applies bloom to the completed scene-linear
HDR image. Implementation lives in `draw/pipeline/BloomPass.cpp`,
`draw/pipeline/DeferredLayer.cpp`, and paired `draw/raster/bloom` and
`draw/raster/final` GLSL/Metal shaders. See the [public API](../api/draw/BloomPass.md)
for settings, signatures and caller obligations.

## Position in the frame graph

```text
raw AO/GI/reflections -> temporal accumulation -> spatial denoise
    -> lighting and ordered HDR transparency/refraction composition
    -> output-sized HDR reconstruction (bilinear / FSR / MetalFX)
    -> bright extraction and reduced bloom pyramid
    -> existing final draw: HDR + intensity * bloom -> exposure -> tone/color
    -> optional FinalProcessor -> overlays -> UI
```

Bloom sees the final composed RGB, including material properties, resolved
lighting, reflections, transparency and sky. Its threshold is independent of the
material emission scalar and camera exposure. It does not modify the HDR input,
G-buffer, raw effects, temporal histories or denoiser guides. No bloom history is
allocated. Existing histories stabilize the input; the filter itself has no
reprojection, temporal rejection or independent stabilization.

`recordLinear()` returns reconstructed HDR without bloom. `recordFinal()` applies
bloom to its supplied HDR image, including an external camera-image mean. In the
[headless accumulation path](draw_offscreen_rendering.md), extraction therefore
runs once on the mean, rather than once per contributing sample. Thresholding is
nonlinear: blooming the mean and averaging individually bloomed frames can differ.

Bloom belongs before tone mapping, not in the post-tone-map `FinalProcessor`
interface. Successful temporal reconstruction may suppress that optional processor
but does not suppress bloom. UI and overlays are added afterwards and cannot seed
bloom. Final debug views, lighting debug views and requested temporal diagnostics
suppress bloom without changing its saved settings.

## Bright extraction and filtering

The default threshold is 1.0. After sanitizing each source texel, extraction uses:

```text
b = max(R, G, B)
bright = RGB * max(b - threshold, 0) / max(b, 1e-6)
```

Exactly-at-threshold pixels contribute zero; any component above the threshold
qualifies, including a saturated primary color with low luminance. Scaling all
components together preserves the sanitized source color ratios. Negative values
become zero, NaNs become zero, and remaining values are clamped to 65504, including
positive infinity. Sanitization affects only bloom, not the scene-color conversion.
There is no soft knee or automatic exposure adaptation.

The first pass extracts individual HDR texels before averaging a 2x2 or 4x4 block.
This avoids losing an isolated bright source to a below-threshold block average.
Further reductions use a five-tap bilinear Dual filter. Reconstruction uses an
eight-tap bilinear filter and fixed-function blending:

```text
fine = (1 - scatter) * retainedFine + scatter * filteredCoarse
```

Source and destination are different images in every draw. Upsampling loads the
retained finer attachment and never samples that destination. Filter and blend
weights are normalized, so constant input does not grow merely by adding levels.
The reconstructed bloom is bilinearly sampled and added in the existing final
shader before exposure and tone/color conversion. No full-screen composition
intermediate is introduced. This is an artistic additive glow; it does not
conserve total scene energy or simulate a measured optical diffraction kernel.

## Memory and slot ownership

Each frame slot owns one dedicated chain of sampled/color-attachment RGBA16F
images. There is no second reconstruction chain, storage-image requirement,
full-resolution bloom target, or reuse of the scene's HDR/G-buffer attachments.
Separate small attachments keep the lifetime and filtering domains explicit.

For divisor `d` (2 or 4), the first extent is `ceil(outputExtent / d)`. Each next
extent is `ceil(previousExtent / 2)`, with a minimum of one per dimension. Creation
stops at the configured level count or 1x1. Border reads clamp to the edge.
At 3840x2160, the default six quarter-resolution levels occupy about **5.273 MiB
per slot**, or **15.818 MiB for three slots**, excluding driver allocation overhead.
Half resolution costs about four times as much. Dimensions refer to width and
height, not pixel count.

Images are allocated lazily on the first enabled record. Stable extents/settings
reuse the chain and per-invocation descriptors. Changes to input extent, initial
resolution or requested level count replace only the acquired, completed slot's
resources. Other slots retain their own chain until acquisition. Replacement builds
the new chain before releasing the old one, so temporary peak memory can exceed
the steady-state estimate. Disabled bloom or zero intensity releases the acquired
slot's chain and descriptors and records no bloom draws. Suppressed diagnostics
follow the same release path. None of these operations submits work or reads back
pixels. Cleanup requires all GPU readers, including final conversion, to complete.

## Settings, capabilities and validation

Defaults are enabled, intensity 0.08, threshold 1.0, scatter 0.7, six levels and
Quarter resolution. The live `final` target exposes the Bloom category in Graphics
Settings and uses the existing preference codec. A bloom-only queued change on
the canonical built-in catalog uses CPU value updates and preserves upstream
histories; resource resizing is deferred to completed-slot recording. Mixed edits
and custom/extended catalogs retain conservative synchronization and invalidation.

Initialization requires sampled/color-attachment RGBA16F with linear filtering
and blending, plus the native shaders. A recoverable runtime initialization error
is caught by `FinalLayer`; ordinary final conversion remains active and
`bloomAvailable()`/`bloomStatus()` explain the result. Direct `BloomPass` callers
receive the initialization exception. Allocation/recording failures are not
silently converted into this initialization fallback. The effect does not require
ray tracing or a temporal-upscaler SDK. GPU profiling labels it `post.bloom`.

Source and numerical-specification inspection do not establish native acceptance.
Validate Vulkan and Metal with threshold values 0.99/1.0/1.01, saturated primaries,
moving subpixel highlights, reflected/transparent emitters, exposure changes,
disabled/zero-intensity output, debug views, odd/tiny extents, resize, Half/Quarter,
1/8 levels, multiple in-flight slots and external HDR accumulation. Measure GPU
time and allocation overhead on target devices; no fixed timing claim is made.
