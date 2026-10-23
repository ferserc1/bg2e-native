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

## Native macOS implementation

The final milestone is implemented in source. The temporary resampling-plus-SMAA
selection path was removed once native MetalFX integration was added.

- Added stable MetalFX method ID 2, preserving None=0 and FSR=1. Metal devices
  retain a disabled native implementation and advertise MetalFX when device/OS
  support is present. The settings target remains the renderer across switches.
- Moved method/mode state into RendererConfiguration so renderer recreation retains
  the chosen mode even while None is selected. Applied the same resolution policy
  on Metal and Vulkan, with exact integer percentage rounding for the five presets.
- Added draw::MetalFXUpscaler and a gpu::metal::MetalFXBridge with no higher-level
  engine dependencies. The bridge uses an Objective-C++ implementation on macOS
  and an unavailable C++ stub elsewhere.
- Weak-linked the MetalFX framework and configured the native source's Objective-C++
  ownership/exception/block options. These build-system and implementation sources
  were authorized by the follow-up request to complete native MetalFX.
- Checks macOS 13+, device support, and reported scale limits on macOS 14+.
  macOS 13 conservatively admits 1x..2x before attempting scaler creation. Native
  AA and 35% are admitted only if limits and scaler creation permit them.
- Unsupported scales/configurations retain the selected mode and internal extent
  and use unjittered bilinear reconstruction with explicit diagnostic status.
- Uses linear HDR RGBA16F, auto exposure, preExposure=1, converted R32F opaque depth,
  and complete RG16F opaque object/camera motion with input-pixel units and Y down.
- Reuses the existing camera and opaque receiver motion shaders. Added paired
  Metal/GLSL depth conversion shaders for D16/D32 input compatibility.
- Added Halton projection jitter, dispatch jitter recovery from recorded matrices,
  history resets, recording-admission checks, per-slot resource retention, and
  asynchronous native command failure detection. Scalers/resources are recreated
  only at initialization or the existing synchronized resize boundary.
- Successful temporal reconstruction uses the renderer's per-slot flag to bypass
  final AA. Failure preserves bilinear output and any injected final processor.
- No automatic SMAA creation or additional graphics controls remain.

Implementation entry points:

- lib/include/bg2e/draw/MetalFXUpscaler.hpp
- lib/src/bg2e/draw/MetalFXUpscaler.cpp
- lib/include/bg2e/gpu/metal/MetalFXBridge.hpp
- lib/src/bg2e/gpu/metal/MetalFXBridge.mm
- lib/src/bg2e/gpu/metal/MetalFXBridge.cpp
- shaders/src/draw/upscale/metalfx_depth.frag.{metal,glsl}
- lib/CMakeLists.txt

## Compatibility and follow-up considerations

- Previously saved `resolution`, writable `upscaler/*`, and `finalProcessor` settings are no longer graphics preference targets. The existing skip-unknown import policy can ignore them; strict imports can reject them. No legacy preference migration was added.
- Unsupported MetalFX modes use bilinear reconstruction at the requested internal extent; the selected mode is not silently changed. No temporal-AA guarantee is made for that fallback.
- None preserves the existing final processor behavior if one was injected. The native implementation does not create or enable a final processor automatically.
- Custom injected upscalers are retained, but are not yet represented as extra entries in the new method selector. Generalize the selection registry when adding further implementations.
- No compilation, tests, runtime execution or validation was performed, as explicitly requested. The Vulkan changes and native MetalFX implementation still need manual review and acceptance on their respective platforms.
