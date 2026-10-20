# Renderer initialization error policy

## Restored quality and first-frame resource consistency

The first `surfaceChanged` may initialize the renderer after surface acquisition.
Preferences restore in the target attachment callback, after allocation of the
default targets. Previously only the next `prepareFrame` rebuilt quality- or
resolution-dependent images. The first frame could therefore reach GI's output
extent validator with mismatched dimensions. AO, reflections and FSR enablement
had the same ordering risk.

`DefaultRenderLoopDelegate::initializeRenderer` now reconfigures immediately after
attachment, before returning to the first update/record. This corrects the
invariant rather than swallowing an exception during command recording.

## Initialization audit

| Area | Recovery / policy |
| --- | --- |
| Stored preferences | Codec validates before applying and reports invalid documents/values; current defaults remain active. |
| Query shadows, AO, GI, reflections | Existing initialization capability and shader/pipeline failures produce unavailable state and neutral/base-lighting paths. Renderer diagnostics now also collect their startup status. |
| Temporal accumulation, denoise, SMAA | Existing unavailable/bypass paths remain active; startup warnings expose failures instead of requiring discovery at record time. |
| Blue noise | Existing hash fallback remains; failure is also included in initialization warnings. |
| FSR | Bridge initialization now lies inside the existing recovery boundary, alongside motion pipeline allocation. Failed FSR uses bilinear reconstruction. |
| Injected upscaler | Runtime initialization failure cleans partial state and selects a fresh bilinear upscaler. |
| Optional final processor / overlays | Runtime initialization failures clean the failed component and omit it from later lifecycle/record calls. |
| Initial generated environment | Runtime generation failure publishes validated, shader-independent black cubemaps and BRDF resources. Subsequent environment updates preserve the existing published resources on failure. |
| Explicit injected environment | Invalid resources remain a contract error; no silent replacement of a caller-owned contract. |
| G-buffer, lighting, final conversion, binding layouts, base targets, bilinear fallback | Required to render a valid scene. Failure remains fatal if these minimum resources cannot be built. |
| Lifecycle misuse, incompatible stages, duplicate/null injections, wrong devices/formats/slots | Programming/configuration contract errors remain exceptions. |

Renderer-level optional recovery catches runtime failures, not every exception.
Cleanup failure or failure to allocate the minimum fallback remains fatal: there
is no established valid state from which rendering could safely continue.
`RendererDeferred::initializationWarnings()` is exposed in the renderer diagnostics
catalog and logged. Component-specific availability/status APIs remain the live
source for feature state. No blanket catch is added around command recording.

Validation: static review and `git diff --check` only. Runtime/compiled regression
checks are listed in `tests/draw/renderer_initialization/README.md`.
