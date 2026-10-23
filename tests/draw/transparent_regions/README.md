# Transparent region verification

`probe.hpp` provides `runTransparentRegionProbe()`, a CPU-only source probe for small coverage, empty offscreen coverage, edge clipping, depth-plane fallback, invalid bounds/matrices, eye-plane fallback and zero extent. It has not been compiled or executed.

For native acceptance, render identical presentations with `RendererDeferred::setTransparentRegionsEnabled(false)` and `true`, resetting histories and replaying the same scene sequence and noise settings. Compare linear HDR and final pixels, including outside each region, on both Metal and Vulkan. Cover overlapping transparent classes, alpha masks, refraction sampling outside the write rectangle, negative/nonuniform transforms, near/far-plane crossings, screen edges, camera/object motion, disappearing layers, resizing, slot reuse and scene replacement. Repeat with shadows, AO, GI and reflections, temporal/denoise enabled and disabled, and debug/custom layers (full-screen fallback). Verify offscreen tracing objects still cast shadows and contribute indirect lighting.

Check complete color clears, a shared preserved opaque depth attachment and disabled depth writes in every transparent G-buffer and metadata invocation; composition must load a complete background copy and preserve its outside pixels. Verify shadow outputs remain neutral beyond their dispatched workgroups. AO/GI/reflection and temporal/denoise outputs retain full-screen validity. Test uploaded bounds after CPU geometry edits and after rebuilding/reloading geometry; changed submesh draw ranges must fall back.

Capture `transparent.fullPixels`, `transparent.rasterPixels`, `transparent.compositionPixels`, `transparent.receiverDomainPixels`, `transparent.backgroundCopyPixels`, `transparent.skippedLayers`, `transparent.fullExtentLayers`, `shadow.receiverInvocations`, and `shadow.fullReceiverPixels`. Receiver invocation counts include workgroup padding. Measure GPU timings separately; full-image copies/clears and temporal/filter dispatches remain real costs. No GPU speedup is claimed without measurements.

The transparent effect receiver pass now covers the complete layer once, before
per-item regional color composition. `transparent.receiverDomainPixels` counts one
full receiver domain and `transparent.effectLayers` is at most one per scene frame.
Compare 1, 4 and 22 visible transparent submeshes at fixed extents and quality after
warming and retiring all slots. Temporal/denoise/metadata image counts must remain
bounded by the two receiver domains (opaque and transparent), rather than grow
with submesh count. Uniform bindings and raster draw work may still grow. Verify
both shared token coverage and the frontmost signal at overlapping pixels, including
objects whose radial-distance order differs from projected camera depth. Preserve
alpha/refraction contributions from objects behind the topmost receiver.

These updated native checks have not been run. No VRAM or speedup result is claimed.
