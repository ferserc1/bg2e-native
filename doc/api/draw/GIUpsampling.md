# Depth- and normal-aware GI upsampling

Implementation: `RendererDeferred::recordGIUpsampling` in
[RendererDeferred.cpp](../../../lib/src/bg2e/draw/RendererDeferred.cpp), with paired
`draw/gi_upsampling/joint_bilateral` compute shaders for Vulkan and Metal.
This is a private renderer stage, not a public `RenderLayer` or an `Upscaler`
implementation. It requires no FSR or MetalFX SDK.

## Purpose and place in the graph

RTGI quality reduces the number of receiver pixels for which rays are traced.
Its output can therefore be smaller than the internal G-buffer. Bilinear sampling
of that signal considers image-space neighbors alone and can mix illumination
from geometrically unrelated surfaces across an edge.

The built-in renderer resolves the GI signal in this order:

1. Trace the diffuse incoming-radiance estimate at GI quality resolution.
2. Apply the configured engine temporal accumulator at the signal extent.
3. Apply the configured denoiser; its output stays separate from temporal history.
4. Joint-bilaterally reconstruct GI to the internal G-buffer extent using the
   current receiver's normals and depth.
5. Pass that reconstructed signal to deferred lighting.

This does not upscale the G-buffer, final color, AO or reflections. FSR/MetalFX or
bilinear full-image reconstruction happens later, after HDR transparency
composition, and maps internal extent to output extent. See
[the rendering model](../../deferred_render_model.md).

## Selection and fallback

The automatic stage requires the exact built-in `pipeline::GBuffer` and
`pipeline::RTGlobalIllumination` dynamic types. Matching formats or inheriting from
these classes does not prove that a custom pass uses the same receiver mapping.
It also requires an active GI graph, GI lighting mode and enabled GI, a retained
resolved signal at the raw GI extent, and a size different from the internal extent.
Full-size GI needs no reconstruction dispatch.

Initialization requires sampled/storage RGBA16F support and the compute shader.
A recoverable initialization failure leaves the original scaled-GI sampling path
in use and appends an `initializationWarnings()` message. This fallback differs
from an individual pixel with no valid compatible GI sample: that pixel has zero
validity, so the existing lighting GI/IBL validity logic applies.

There is currently no public enable switch, settings target or threshold API for
this stage. GI quality controls its input resolution; the filter constants below
are private implementation choices in scene units.

## Inputs, formats and receiver mapping

| Image | Extent | Format / role |
|-------|--------|---------------|
| Resolved GI | GI effect extent | Sampled RGBA16F; RGB is radiance, alpha marks validity |
| Normal guide | Internal | Sampled RGBA8 UNORM or RGBA16F, decoded from RGB × 2 − 1 |
| Depth guide | Internal | Sampled D16 UNORM or D32 float, non-reversed zero-to-one camera depth |
| Output | Internal | Distinct sampled/storage RGBA16F image |

All images must be valid 2D images on the same Device. Output cannot alias any
input. Position is reconstructed with the inverse of the **jittered**
view-projection that generated these receivers; world-to-view supplies depth for
the relative tolerance. The stage uses nearest texel access for guides and signal.

For low-resolution texel `q`, its associated internal receiver is
`min(floor((q + 0.5) / signalExtent * internalExtent), internalExtent - 1)`.
This preserves the built-in RTGI producer's receiver selection, including odd
extents. Guessing a different half-pixel convention would associate a radiance
sample with the wrong surface near edges.

## Filter algorithm

Each output invocation examines a 4×4 neighborhood around its continuous position
in the low-resolution signal. A receiver is invalid for nonfinite/degenerate
normal data, invalid reconstruction, or depth outside [0, 1); clear far depth is
background. Invalid destination receivers write `(0,0,0,0)`.

A candidate must have finite RGB/alpha and alpha greater than 0.5. Its mapped
receiver must also be valid. Let `d` be the world-space displacement between the
candidate and destination, with normals `nq` and `np`. The geometric separation is
`max(abs(dot(d, np)), abs(dot(d, nq)))`. This measures separation from both tangent
planes, rather than raw camera-depth difference, so coplanar surfaces remain
compatible at grazing view angles.

The candidate must meet both tests:

- Separation ≤ `max(0.01, 0.01 * max(abs(candidateViewZ), abs(destinationViewZ)))`.
- Normal cosine ≥ 0.9.

Accepted nonnegative radiance is weighted with a Gaussian combining spatial
distance in signal-pixel units, plane separation and normal difference. Current
spatial sigma is 1.0, depth sigma is 0.5 times the geometric tolerance and normal
sigma is 0.1 in `(1 - cosine)` units. A finite normalized weighted sum is preferred;
if its weight is too small (≤ 1e-8) or arithmetic is unusable, the nearest accepted
sample is used. This fallback never relaxes the geometric tests. Ties follow stable
loop order. If no candidate passes, output remains zero with alpha zero.

Valid output has alpha one and RGB clamped to [0, 65504], the finite positive
RGBA16F range. Valid black therefore remains distinguishable from missing GI.
The filter reduces edge leakage; it cannot invent missing thin-surface samples or
guarantee leakage-free reconstruction for all scene scales.

## Opaque and transparent execution

The stage may run once for opaque domain 0 and once for shared transparent domain
`RendererDeferred::TransparentTemporalLayer` (1), within the same acquired slot.
It uses the actual receiver scratch depth `gbuffer[6]` paired with `gbuffer[1]`
normals. In particular, transparent receivers in front of sky use captured
transparent depth, not far-cleared preserved opaque depth.

Both domains reuse one internal-sized output target sequentially. Opaque lighting
consumes its result before transparent effects overwrite that target. The shared
transparent GI result is then reused during ordered submesh composition. This is
not one upsampled image or history per transparent object. Full-image FSR/MetalFX
still receives **preserved opaque depth** and optional **opaque** motion metadata.

## Recording and lifetime

A 16×16 compute dispatch rounds group counts up, with shader bounds checks for odd
extents. Recording happens outside active render/compute scopes. Inputs transition
to shader-readable layouts, output to storage/general, and output back to
shader-readable after the compute pass. These dependencies order GPU accesses;
they are not CPU completion waits.

Each invocation retains its input/output owners, a distinct 176-byte uniform
buffer and a resource set until acquired-slot retirement. Uniforms contain two
matrices, input/output extents and filter constants. Vulkan set 0 uses UBO binding
0, sampled signal/normal/depth bindings 1/2/3, sampler 4 and storage output 5.
Metal maps these to buffer 1, textures 0/1/2, sampler 0 and output texture 3.
No descriptor/uniform storage is overwritten between the opaque and transparent
invocations. Duplicate layer invocation or a non-retired/mismatched slot rejects.
Attach/resize reset slots only after the lifecycle drain; cleanup uses that same
drained boundary. Recording does not add an idle wait or immediate submission.

These are source contracts, not a claim of native visual/performance acceptance.
