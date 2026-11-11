# Buffer inspection verification

Run from the repository root:

```sh
python3 tests/draw/buffer_inspection/reference.py
python3 tests/draw/temporal/phase3_source_checks.py
python3 tests/draw/temporal/integration_source_checks.py
```

Native verification is pending; no build or shader compilation is performed by
these checks. After an explicitly authorized build, use Composer on Vulkan and
Metal with a cube over a ground plane and at least one transparent object:

- Open Window > Debug: it starts disabled and its remaining controls are disabled.
- Enable it; inspect every G-buffer and switch RGB/R/G/B/A. Check nearest zoom,
  fixed exposure and the depth range. A source size is reported for available views.
- With FSR/MetalFX active, compare GI Original, Accumulated and Filtered without
  moving the camera. The normal upscale pass must continue in a GPU capture.
- Compare Before/After Upscale with the same debug exposure. Inspect radiance and
  reflection confidence independently; exercise AO mode and shadow indices.
- Select opaque/transparent layers. The transparent view denotes the last receiver.
  Disable the transparent layer: it must disappear from direct rendering, shadows,
  reflected hits and indirect transport, without changing the scene tree or dirty flag.
- Request an unavailable effect/temporal diagnostic: show the unavailable message
  and purple placeholder rather than an old frame or a fabricated signal.
- Resize, switch upscalers and change scenes while debugging. No image from an old
  extent/slot may be read. Unsupported renderer/shader paths must report a reason.
- Disable Debug and close/reopen the window. No inspection passes or snapshot
  images remain; temporal color histories are preserved unless visibility changed.
  Verify this with a GPU resource capture, not just process memory (driver caches
  may retain allocations). Repeat on both backends and with multiple frames in flight.
- Restart Composer: Debug starts disabled regardless of its previous session state.

No exact frame counts, screenshot export or externally timed invalidation is needed
for visual inspection. Check compilation, viewport behavior and resource lifetimes
separately; CPU/source checks do not establish these native results.
