# Accumulation, gutter dilation and readback

## Per-target resources

`LightmapBaker` retains its context, target and immutable `LightmapSettings`.
It owns one `UvSurfacePass`, one `UvTemporalAccumulator`, and per-frame-slot AO
(R8), GI (RGBA16F) and composed sample (RGBA16F) images. The surface projection
is also per frame slot. The temporal accumulator instead owns two RGBA32F history
images and two RGBA32F dilation images independent of frame slots.

This separates transient frame resources from persistent atlas history. A slot
can be safely recycled after its frame fence while the history continues across
ordered graphics-queue submissions. Image transitions/barriers establish
write-to-read dependencies; sharing history does not imply concurrent unordered
updates are supported. History is initialized by its first write, and the active
ping-pong index is CPU bookkeeping reflecting recorded work.

Sources: [baker](../../../lib/src/bg2e/render/LightmapBaker.cpp),
[accumulator header](../../../lib/src/bg2e/render/UvTemporalAccumulator.hpp),
[implementation](../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp).

## Same-texel progressive mean

UV addressing is stable, so no screen-space reprojection, camera history,
motion-vector search or disocclusion rejection is needed. `record()` receives
`previousSamples = completedFrames` before marking the next result. At each
valid texel, for composed sample \(S\) and previous mean \(H_n\):

\[
H_1=S,\qquad H_{n+1}=H_n+\frac{S-H_n}{n+1}\quad(n>0).
\]

Here `n` is previous update count, not the number of rays inside that update.
Each update contributes one equally weighted per-texel sample image; requested
rays per covered texel across a completed bake are
`accumulationFrames * samplesPerPixel` (GI may trace several bounces per ray).

```glsl
if (texelFetch(validTexelMask, pixel, 0).r == 0u) {
    imageStore(nextHistory, pixel, vec4(1.0, 1.0, 1.0, 0.0));
    return;
}
vec3 sampleValue = texelFetch(composedSample, pixel, 0).rgb;
vec3 value = sampleValue;
if (pc.previousSamples != 0u) {
    vec3 previous = texelFetch(previousHistory, pixel, 0).rgb;
    value = previous + (sampleValue - previous) / float(pc.previousSamples + 1u);
}
imageStore(nextHistory, pixel, vec4(value, 1.0));
```

Source: [lightmap_accumulation.comp.glsl](../../../shaders/src/glsl/lightmap_accumulation.comp.glsl).
Inputs use exact `texelFetch`, not bilinear sampling across island boundaries.
Alpha is one only where the surface mask is valid. Empty history is white with
zero alpha. The first dispatch binds a valid sample descriptor in place of old
history; the shader does not read the history branch for zero previous samples.

`resetAccumulation()` clears progress/result flags; `UvTemporalAccumulator::reset()`
invalidates logical history without clearing GPU memory. The next sample writes
fresh values and ignores old history. A geometry/UV/material/environment change
cannot be mixed safely into a converging mean. Recreate bake resources after
UV topology changes; standalone `updateScene()` resets tracked live bakers when
scene inputs change.

## Dilation as a separate image operation

After each mean update, `dilationPixels` dispatches expand covered RGB into empty
atlas gutter. History itself remains unchanged. The first dilation reads the
new history; subsequent passes alternate the two dilation images. This prevents
GPU workgroup execution order from influencing neighbor reads.

Covered alpha `>0.5` is preserved. For an empty texel, the shader examines its
3×3 neighborhood and picks a covered candidate with smallest `abs(dx)+abs(dy)`.
Cardinal neighbors take priority over diagonal ones. Equal-distance ties keep
the first encountered candidate in the shader's y/x loop. Copied alpha makes
the new gutter texel eligible on the next pass. There is no blending of neighbors
or knowledge of chart identity in this shader.

A pass can grow diagonally as well as cardinally, so the repeated expansion is
not an exact Euclidean distance transform. Chart gaps can be filled from either
side according to local traversal ties. Padding sets separation during packing;
dilation fills color afterwards to reduce interpolation with empty white texels.
It does not regenerate UVs or add light samples. The valid range is 1–32,
including no public zero-pass option.

Sources: [dispatch/ping-pong](../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp),
[dilation shader](../../../shaders/src/glsl/lightmap_dilation.comp.glsl).

## Result access and conversion

`image()` exposes the final dilated RGBA32F image in shader-read layout. A shared
pointer retains storage but does not freeze it; later updates may reuse it.
Integrated marking happens during command recording, so image availability is
not GPU completion. `readPixels()` refuses a result from the engine's unchanged
frame number, waits for device idle and performs an immediate transfer to a
GPU-to-CPU staging buffer. It restores the image layout and invalidates the
mapped allocation before reading.

`LightmapPixels` strips alpha and returns row-major RGB. RGB32F copies floats
unchanged. RGB8 multiplies finite channels by `2^exposureEV`, clamps to `[0,1]`,
multiplies by 255 and rounds. Nonfinite RGB8 source values become zero. Exposure
is passed as zero for RTAO, regardless of settings; RTGI GPU/RGB32F values are
never exposed by this conversion. There is no readback gamma encoding.

Exporting RGB32F through `LightmapOutputWriter` still creates an 8-bit file:
that writer validates finiteness and clamps/rounds without exposure. RGB32F is a
CPU format, not HDR file-format support.

Sources: [result guards](../../../lib/src/bg2e/render/LightmapBaker.cpp),
[readback/conversion](../../../lib/src/bg2e/render/UvTemporalAccumulator.cpp),
[output writer](../../../lib/src/bg2e/db/LightmapOutputWriter.cpp).
See [contexts](contexts.md) for submission and [output](output_and_materials.md)
for the pending dedicated material channel.
