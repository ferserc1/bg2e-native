# MotionVectorGenerator and TemporalView

Header: `<bg2e/draw/pipeline/MotionVectorGenerator.hpp>`.
Namespace: `bg2e::draw::pipeline`. This is a CPU camera-motion reference, usable
without Engine or scene-private resources; it is not an image-producing pass.

TemporalView stores view, projection, jitteredProjection and reproject=true.
validate(view) checks its matrices. camera(current,previous) returns
MotionVectorTransforms with currentFromJittered and previousFromJittered.
motionPixels(transforms,currentJitteredClip,extent) returns previous minus current
unjittered framebuffer pixels; framebuffer Y opposes clip Y. Jitter-only changes
produce zero unjittered camera motion. Keep current/previous conventions consistent
with the zero-to-one depth and negative clip-Y framebuffer mapping.

The temporal GPU kernels use previous jittered projection to look up history;
this reference does not replace FsrUpscaler's existing camera-motion image pass.
There are no previous-object transforms/vectors here. Notify Renderer::invalidateHistories
on geometry motion, camera cuts or custom in-place changes; the built-in renderer
also detects extraction/resource/settings changes and conservatively resets.

For stationary explicit surfaces, TemporalView::reproject=false requests same-UV
correspondence. It does not turn display history into an exact baking accumulator.
See [TemporalAccumulator](TemporalAccumulator.md) and [surface inputs](SurfaceInputs.md).
