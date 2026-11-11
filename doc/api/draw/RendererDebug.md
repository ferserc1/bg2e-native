# Session-only buffer inspection

Composer exposes **Window > Debug**. The first checkbox is **Enabled**; every
inspection control is disabled while it is off. Closing the window disables
inspection. This state belongs to the UI/renderer session, is not registered in
the graphics preferences catalog, and is never written to scene or user settings.

Select one buffer in the main viewport. G-buffer views include albedo, encoded
normals, material channels, roughness, metallic, Fresnel/flags, sheen/refraction,
baked multiplier and linear depth. Lighting views include emission, direct,
indirect, irradiance and prefiltered specular. AO, GI, reflections and reflection
confidence provide Original, Accumulated and Filtered stages. Filtered includes
spatial GI upsampling when active. Shadow visibility selects a shadow index.
Temporal acceptance, motion, current weight, effective count, reactivity and
variance use the existing optional diagnostic interfaces and receiver metadata.
They report unavailable if their effect, metadata or adaptive diagnostic path is
not active; enable the applicable capabilities in Graphics Settings.

Opaque and transparent receiver layers can be inspected independently. Transparent
inspection means the last processed transparent receiver, not an aggregate of all
transparent surfaces. Before/After Upscale are whole-scene compositions and do not
use this layer selector. The **Disable transparent layer** option excludes both
transparent queue classes before correspondence, object bindings and tracing
snapshot preparation. It affects rasterization, shadows, GI and reflections;
opaque frame object identifiers are reassigned consecutively. Toggling this
visibility option invalidates rendering histories without changing scene objects.

Channels select RGB, R, G, B or A. Debug exposure is fixed and independent of the
normal final color settings. HDR values survive snapshot conversion before debug
exposure; display values clip to [0,1]. Linear depth has its own minimum/maximum
in camera-space units. Nearest zoom enlarges the center of the snapshot without
interpolation. Unavailable selections show a purple viewport and a status message.
Missing diagnostic shaders leave normal rendering visible and report the error.

## Renderer contract and cost

`RendererDeferred::setDebugOptions(RendererDebugOptions)` is a synchronized,
nonpersistent update. Composer uses `enqueueRendererUpdate`, including a renderer
generation check. `debugOptions()` and `debugStatus()` expose current CPU state.
Input enums, finite values, channel, stage, layer and depth ranges are validated.

The normal graph and its jitter, effect accumulation and temporal upscaler remain
active. Inspection never feeds a selected debug image into FSR/MetalFX. Only after
normal final processing and overlays does an independent display pass replace the
viewport image. Selecting another view does not invalidate effect histories;
changing transparent visibility does. Existing standalone renderer diagnostics
retain their normal behavior outside this inspection facility.

No inspection state, image or pass is allocated when disabled. With a non-final
view enabled, resources are created lazily: one RGBA16F snapshot per acquired frame
slot (8 * outputWidth * outputHeight bytes each), plus shader pipelines/bindings.
Only the selected signal is converted immediately at its stage, before scratch
storage can be reused by transparent processing. Source images need no persistent
copies or CPU readback. Component lighting views additionally replay deferred
lighting into one internal-resolution RGBA16F target per used slot; the original
lighting debug setting is restored even on exceptions.

Disabling inspection or returning to Final releases snapshot resources after GPU
users retire. If temporal diagnostic captures were requested, teardown also drops
retired diagnostic invocation/binding caches and acceptance/metric images while
preserving accepted signal/geometry/moment history owners. Resize and cleanup
release inspection resources. The disabled render path only tests selection flags;
it performs no extra GPU work or diagnostic image retention.

## Verification

`python3 tests/draw/buffer_inspection/reference.py` checks UI/view schema agreement,
nonpersistent scheduling, stage ordering, unchanged upscale input, disabled resource
ownership, transparent exclusion/identifier invariants, HDR preservation and linear
depth reference math. These are CPU/source checks, not native rendering evidence.
See the manual checks in `tests/draw/buffer_inspection/README.md`.
