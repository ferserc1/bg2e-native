# MetalFX implementation handoff

## Requested behavior

Graphics settings must expose one upscale method selector and one mode selector. Windows/Linux use FSR through Vulkan. macOS must offer MetalFX Temporal Upscaling. Future implementations should fit the same selection model.

| Mode | Internal width and height relative to output |
| --- | --- |
| Native (AA) | 100% |
| Quality | 60% |
| Balanced | 55% |
| Performance | 50% |
| Ultra Performance | 35% |

Percentages apply independently to both dimensions, rounded down with a minimum of one pixel. None uses native output resolution; the selected mode is retained for later reactivation.

## Vulkan work implemented in this change

- Reused the existing FSR 3.1 Vulkan implementation, FidelityFX bridge, projection jitter, opaque depth and opaque object/camera motion path.
- Added `RendererDeferred::UpscaleMethod` and `UpscaleMode` with reflected settings in the `upscaling` component (`bg2e.draw.upscaling.v1`).
- Vulkan renderers without an injected upscaler now retain a disabled FSR implementation so users can select FSR at runtime. Explicitly injected alternative upscalers retain their implementation.
- Method changes toggle the retained FSR implementation. Mode changes update `ResolutionSettings`; the existing delegate extent/reconfiguration path resizes resources and rebuilds FSR at its synchronized boundary.
- Native selects full-resolution FSR temporal reconstruction/AA. FSR sharpness remains at its programmatic value; it is no longer a separate graphics control.
- Removed the independent resolution and upscaler controls from the settings catalog and stopped exposing the final processor as a separate graphics setting.
- Final post-tone-map processing is skipped only when temporal reconstruction succeeded for that frame slot. It remains a fallback when reconstruction is unavailable or fails.
- Upscaler capability metadata remains available to diagnostics.

Main integration points:

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `lib/src/bg2e/draw/RendererDeferred.cpp`
- `lib/src/bg2e/draw/RenderSettingsPreferences.cpp`
- Existing interfaces: `draw/Upscaler.hpp`, `draw/FsrUpscaler.hpp`, `gpu/vk/FidelityFXBridge.hpp`.

## Remaining macOS work

### First milestone: reduced resolution plus existing AA

1. Extend the method enum with a stable MetalFX ID without renumbering None (0) or FSR (1). Offer None / MetalFX on Metal devices; do not advertise an operational MetalFX temporal implementation before it exists.
2. Generalize `upscaleMethod()` and `setUpscaleMethod()` beyond their current FSR-specific dynamic cast and enabled flag. Preserve a stable settings target while switching methods.
3. Apply the same mode percentages on Metal, including Native at 100% and None at native output extent. Vulkan initialization currently normalizes resolution policy; add the corresponding Metal policy and preserve the chosen mode across renderer recreation.
4. Implement a clearly identified temporary MetalFX selection path using the existing `ResamplingUpscaler` and a final AA processor. Merely selecting the current resampler does not create or enable AA: ensure a compatible processor is instantiated and enabled.
5. Keep this temporary path unjittered. Leave `temporalUpscaleApplied` false so the final AA processor runs after tone mapping and before overlays/UI.
6. Keep graphics settings limited to Upscale and Mode, without restoring the previous independent resolution, sharpness, or AA toggles.

### Final milestone: native MetalFX temporal reconstruction

1. Implement a `draw::Upscaler` backed by MetalFX temporal scaling and the required low-level Metal bridge. Keep higher-level draw/scene dependencies out of `bg2e::gpu`.
2. Check OS/device/API support and supported scale limits, including whether 35% and native-resolution temporal AA are supported. Define an explicit fallback when a requested mode cannot run.
3. Integrate the MetalFX framework and Objective-C++ backend sources into the macOS build as needed. No CMake files were changed in this task; obtain authorization for build-system changes when continuing.
4. Adapt the linear HDR input/output contract to MetalFX texture format, usage, exposure and color-processing requirements. Reconstruction currently runs before tone mapping.
5. Supply matching opaque depth and motion vectors, with correct Metal coordinate conventions, jitter conventions, motion direction/units and depth convention. Reuse the opaque receiver metadata and camera/depth fallback where appropriate.
6. Implement projection jitter through `prepare()`, submit frame inputs through `record()`, and report accurate availability/status and resource requirements.
7. Maintain temporal history and reset it on camera cuts, projection changes, discontinuous scene frames, resize/mode changes and abandoned command recordings. Retain per-slot resources through GPU completion.
8. Recreate the MetalFX scaler and extent-dependent resources only at the existing synchronized lifecycle boundary. Handle partial initialization, unsupported devices and runtime failures with a usable fallback.
9. Return successful temporal reconstruction through the renderer's per-slot `temporalUpscaleApplied` flag so the old final AA pass is bypassed.
10. Once real MetalFX is complete, remove the temporary reduced-resolution-plus-AA substitution and its automatic AA setup. Keep the user-facing selectors unchanged.

## Compatibility and follow-up considerations

- Previously saved `resolution`, writable `upscaler/*`, and `finalProcessor` settings are no longer graphics preference targets. The existing skip-unknown import policy can ignore them; strict imports can reject them. No legacy preference migration was added.
- macOS currently exposes only None for the built-in resampler. MetalFX is intentionally left for the macOS implementation.
- None preserves the existing final processor behavior if one was injected. This task does not create or enable a final processor automatically.
- Custom injected upscalers are retained, but are not yet represented as extra entries in the new method selector. Generalize the selection registry when adding further implementations.
- No compilation, tests, runtime execution or validation was performed, as explicitly requested. The Vulkan changes and macOS requirements still need manual feedback on their respective platforms.
