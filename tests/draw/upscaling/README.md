# M01-13 manual validation

No executable or CMake target is added. The project lead compiles the library,
paired draw shaders and existing production/draw examples; use the existing
`tests/draw/deferred` fixture/host with the following configuration changes.
These probes have not been compiled or executed by the implementing agent.

1. Default configuration: native output must remain equivalent. Enable the live
   `resolution` target and set `scale` to 0.5: G-buffer, opaque depth and HDR
   composition become half-size; reconstructed HDR and final output remain full
   size. Try 0.25, 0.59, 0.67, 1 and odd output sizes (1001x701), including 1x1.
   Reject NaN, infinity, zero and scales outside [0.25,1]. Disabled scaling must
   restore the original explicit fixed extent or follow-output policy, even after
   output resize and surface format replacement. No MSAA or engine histories.
2. Compare all debug views at native/reduced resolutions, especially depth; debug
   sampling must use the G-buffer size rather than reconstructed color size.
   Verify exposure/gamma/brightness/contrast and UNORM/sRGB parity apply once.
3. Set `configuration.upscaler = std::make_shared<FsrUpscaler>()`. On supported
   Vulkan, expect FSR available, SDK/backend/hardware status separately exposed,
   camera-motion vectors, scene-frame jitter and sharpen control. Check static
   edges, camera translation/rotation, sky, foreground, cutouts and glass. Inspect
   motion sign: previous framebuffer pixel minus current unjittered framebuffer
   pixel; stationary scene/camera produces zero motion despite projection jitter.
   Shader pushes are two matrices (128 bytes); negative-height Vulkan viewport
   and Metal framebuffer convention both invert NDC Y.
4. Disable FSR through `upscaler/bg2e.draw.fsr3.v1/enabled`, then re-enable via the
   queued catalog update. Render extent stays reduced; disable shows unjittered
   bilinear output. Re-enable rebuilds SDK context and resets. Unsupported SDK,
   Metal, HDR storage/capability/context/kernel failure, orthographic/identity
   cameras and more than four in-flight slots retain bilinear output and expose
   a reason. Retry resource failures with a disabled/enabled boundary or renderer
   replacement; ordinary frames must not repeatedly allocate/retry.
5. Scene attachment, renderer/extent/format replacement, projection changes,
   explicit `resetUpscaler()` after camera cuts and skipped scene-frame numbers
   reset FSR. UI-only presentations do not advance SDK state. Acquire multiple
   slots with validation layers: output/motion/descriptors are slot-owned; SDK
   internal/shared resources follow one ordered graphics stream. There must be
   no ordinary per-frame idle, immediate submit or readback.
6. Inject `ApplicationUpscaler` from `probe.hpp`. Preferences must enumerate
   `upscaler/application.resampler.v1` without registration/engine enum edits.
   Toggle its live `enabled` property; its call counter stops when bypassed and
   built-in fallback still draws. Export/import settings with SettingsCodec;
   stale handles/imports must fail after graph resize/replacement. Preserve all
   FinalLayer live settings. Test a custom implementation whose needsRebuild
   returns true after settings edits; detach precedes resize, rebind follows it.
7. Run the earlier deferred pixel/transparent/IBL fixtures and production
   examples. Injected final passes use
   `pipeline::finalContract(outputFormat, depthFormat, true)` in RendererDeferred:
   color is output-sized HDR; seven debug inputs remain internal-sized. Direct
   standalone FinalLayer users can retain the default internal-color contract.

Historical M01-13 limits above have evolved: the optional Phase 2 receiver path
now supplies rigid opaque object motion, while absent/unsupported metadata keeps
camera-only motion. Conventional RH zero-to-one perspective, no reactive mask,
deformation/transparent/reflected-content motion limitations still apply. Native
Metal FSR and DLSS remain unsupported extension cases. Current source status follows.


## P2.4 source fixture and native acceptance still pending

Include `opaque_motion_probe.hpp` and call `runOpaqueMotionProbe(engine, shaderRoot)`
only in a stopped initialized host after build authorization. It needs no FidelityFX
SDK or RT hardware: it renders the legacy and optional selection shaders directly.
False is unavailable shader/format support, never success. It checks odd 5x3 output,
integer tokens including UINT32_MAX, unchanged (-3,4)/(.5,-.5) vectors, valid zero
motion overriding nonzero camera motion, token-zero and NaN/Inf fallback, camera
pixel units/Y sign, ordinary/sky depth and eight malformed optional input cases.
It uses actual raster/readback only within the native fixture; it has not run.

Optional UpscalerOpaqueMotion contains borrowed RG16F motion/R32_UINT token pointers
plus optional matching owners. FsrUpscaler validates and retains supplied owners per
invocation in retired-slot rings. Renderer input must come from the opaque pair,
never the final glass layer; enable receiver replay explicitly to generate it.
The new pass reuses the existing motion image and 128-byte camera push. SDK scale
is (1,1), internally divided by render size, with unjittered previous-current pixels.
The bridge/vendor sources and old camera shader are unchanged.

When an SDK-supported native session is authorized, additionally test two/three
queued slots, object/camera translations and rotations, valid zero vectors despite
jitter, metadata disabled/unavailable and shader-missing fallback, stationary sky,
opaque metadata after multiple transparent layers, retained image owners after slot
reuse, abandoned recordings/scene gaps/cuts/projection changes and resize. Prepare
another slot before recording the first and verify SDK jitter still matches that
first scene's captured projection. Check no extra resets on transform-only movement
or metadata presence changes. Compare FSR trails against camera-only fallback;
transparent/refraction/reflection correspondence remains approximate. There must be
no ordinary waits/submissions/readbacks. SDK unsupported and Metal FSR are fallback,
not passes. C++/shader compilation and all native/pixel/performance checks remain
not_run; P2.5 will consolidate regression/source acceptance without implying runtime.
