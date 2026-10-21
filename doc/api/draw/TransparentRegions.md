# Transparent processing regions

The default deferred renderer limits transparent G-buffer draws and lighting composition to conservative projected bounds of uploaded rigid submeshes. Sorting remains stable and far-to-near by object origin. Offscreen transparent raster layers can be skipped without removing tracing geometry.

Use `RendererDeferred::setTransparentRegionsEnabled(false)` for a full-screen diagnostic baseline. Uncertain bounds, depth-plane intersections, non-affine transforms, custom raster/lighting/final passes and debug views automatically retain that baseline. Regions use the jittered camera and include a raster edge guard.

G-buffer clears and depth restoration retain full extent. Regional composition first copies the complete current background to the other ping-pong target and loads it, preserving outside pixels and arbitrary refraction reads. `pipeline::RasterFrame::region` is borrowed pass coverage; replacement passes receive full coverage from this renderer. `FullscreenPass::draw` accepts an optional trailing region; direct callers must initialize the complete destination before a regional draw.

`gpu::RenderingDescription::scissor` uses top-left attachment pixel coordinates. It restricts graphics draws, preserves the full viewport and attachment clear area, and is restored at each pipeline binding. Empty/out-of-bounds scissors are rejected. This facility is implemented in both GPU backends used by production draw.

Pointwise soft shadows can omit right/bottom receiver workgroups beyond a conservative origin-anchored region and clear untouched output to neutral visibility. AO/GI/reflections, temporal accumulation and denoise keep complete signal/history processing. Filter and reprojection validity therefore retain the existing full-screen contract.

Area counters describe raster/composition coverage and shadow workgroup invocations, rather than GPU timing. Complete image copies/clears remain costs. See `tests/draw/transparent_regions/README.md` for comparison procedure and counters. Native parity and GPU performance acceptance are pending.
