# Live graphics settings and upscaling

Headers: `<bg2e/draw/SettingsCatalog.hpp>`, `<bg2e/draw/SettingsCodec.hpp>`,
`<bg2e/draw/RenderSettingsPreferences.hpp>`, `<bg2e/draw/Upscaler.hpp>`,
`<bg2e/draw/FsrUpscaler.hpp>`, `<bg2e/draw/MetalFXUpscaler.hpp>`, `<bg2e/ui/GraphicsSettingsWindow.hpp>`.

## Live values and safe changes

`SettingsCatalog` stores reflected metadata and live accessors, not a second
settings model. `SettingsTarget` carries a stable component ID, TypeInfo,
instance address, optional owner, availability predicate, proposed-value validator
and post-commit changed callback. `properties()` enumerates descriptors;
`read(handle)` calls the live getter. `SettingHandle` contains generation,
component and property. Reset/rebind invalidates old handles. Clear borrowed
catalog targets before destroying their instances.

`validate`/`apply` enforce exact std::any types, ranges, finite numbers and enum
identities. Imports validate the complete proposed change set before writes;
setter failure/rollback cannot make arbitrary external side effects transactional.
Stable renderer/component/property IDs are serialization identities, independent
of UI labels and C++ addresses. Application passes/upscalers supply their own live
metadata; no closed engine effect/upscaler enumeration is required.

`RenderSettingsPreferences` is a shared live facade with catalog/lifecycle state
and no duplicate editable settings-value model. Optional
`app::Preferences*` persistence is borrowed. `connect(shared_delegate, extensions)`
installs weak attach/detach callbacks before first renderer binding. `queueSet`,
`queueImport` and `queueRestore` commit through the delegate's pre-acquisition
safe boundary and reject stale renderer/catalog generations. Direct bind/apply/
restore require a stopped boundary. Automatic persistence is disabled by default.
`setAutomaticPersistence(true)` opts into a one-time restore on first renderer
attachment and persistence of successful queued edits/imports; install it before
attachment when that restore is wanted. Enabling it requires a borrowed
Preferences service. Explicit settings documents bypass stored
defaults; absent properties retain current live values.
Preferences retains its existing log-only disk durability reporting.

Use `SettingsCodec::readFile`, `exportFile`, `parse` and `stringify` for strict
settings JSON. Numeric payloads use typed decimal strings because ordinary
JsonNode numbers store float and cannot preserve all uint32/double/int64 values.
Do not replace the codec with generic JSON-number serialization. Unknown or
unavailable properties reject by default; `SkipUnknownOrUnavailable` is explicit.

`ui::GraphicsSettingsWindow::bind(facade)` reads fresh descriptors/getters each
frame. Edits/import/restore queue changes; pending edits block export/persist.
Its diagnostics and native editor IDs follow catalog generations, while
availability refreshes independently. `ReflectionWidget::editValue` uses transient
proxies without following raw object pointers. Draw material, texture and picking widgets share this production UI session.
Use ResourcePicker for HDR selection and clear preview owners before teardown.

## Resolution and optional reconstruction

`RendererDeferred` exposes `UpscaleMethod::{None, FSR, MetalFX}` and
`UpscaleMode::{Native, Quality, Balanced, Performance, UltraPerformance}`. The
unified method/mode controls own internal resolution on Vulkan and Metal.
Initialization enables `ResolutionSettings` and sets scale to 1.0 for None, or to
the selected mode's 1.0/0.60/0.55/0.50/0.35 ratio. `setUpscaleMethod` also reapplies
the mode; `setUpscaleMode` updates the policy and invalidates histories. A bare
`ResolutionSettings` object still defaults disabled/1.0 and accepts [0.25, 1], but
those standalone defaults do not describe the initialized deferred renderer.

Preset dimensions use exact integer percentages, rounded down independently with a
one-pixel minimum. None retains the selected mode while rendering at scale 1.0.
Native requests same-size temporal AA if supported. These ratios are engine
presets, not the SDKs' standard quality labels. See [the resolution example and
pipeline](../../deferred_render_model.md#three-resolution-domains).

The live catalog exposes target `upscaling` with properties `method` and `mode`.
Its method enum includes only selectors compatible with the current renderer.
The `upscaler/<identifier>` target exposes implementation capability diagnostics;
the unified graphics controls do not expose every implementation-specific writable
accessor (for example FSR sharpness).

A null upscaler retains a backend implementation (disabled FSR on Vulkan, disabled
MetalFX on Metal). An unset method derives from the enabled state of an injected
built-in implementation; otherwise it selects None. An explicitly incompatible
backend/method/upscaler combination rejects. `supportsUpscaleMethod` checks selector
compatibility (and MetalFX device support), not successful reconstruction at every
extent. Check the component's `available()` and `status()` as well.

`ResamplingUpscaler` supplies output-sized linear HDR RGBA16F bilinear reconstruction
when the active implementation is unavailable or returns false. The selected mode
and internal extent remain in effect on fallback. Temporal implementations return
an unjittered projection when they already know the configuration/camera is
unsupported. Tone/color conversion runs once afterwards. Successful temporal
reconstruction bypasses the optional final processor, including SMAA; bilinear
fallback may run an explicitly injected processor.

FSR uses the optional Vulkan FidelityFX bridge (at most four queued SDK frames).
MetalFX uses the native Metal temporal scaler on supported macOS 13+ devices and
checks the requested scale; native AA and UltraPerformance are not guaranteed.
Both use preserved opaque depth, camera motion and optional valid opaque object
motion. Conventional finite perspective cameras are supported; unsupported
projections use unjittered fallback. Receiver replay is opt-in/default off.
Transparency/refraction and reflected motion remain approximate. No DLSS or frame
generation is implemented by these paths.

[Upscaler](Upscaler.md) documents SDK diagnostics, input lifetime, motion conventions
and reset conditions. [GI upsampling](GIUpsampling.md) is a separate guided
low-resolution-effect stage, controlled by GI quality rather than Upscale Mode.

See [working tutorial](../../tutorials/draw/10_deferred_renderer.md).

## Implemented effect targets and diagnostics

| Target | Requested properties / meaning |
|---|---|
| renderer | indirectLighting enum IBL=0, AmbientOcclusion=1, GlobalIllumination=2; inspectOpaqueLayer |
| ambientOcclusion | enabled, quality, sampleCount, bounceCount, radius, bias, falloff, bounceAttenuation, useBlueNoise |
| globalIllumination | enabled, quality, sampleCount, bounceCount, bias, maxDistance, shadowSamples, useShadows, useBlueNoise |
| reflections | enabled, quality, sampleCount, bias, maxDistance, maxRoughness, roughnessSpread, shadowSamples, useShadows, useBlueNoise |
| temporal | enabled, freezeAtLimit, maxFrames, absolute/relativePositionThreshold, normalThreshold, adaptiveEnabled, minCurrentWeight, reactiveCurrentWeight, historyClipSigma |
| denoise | enabled, kernelRadius, depthThreshold, normalThreshold, depthSigma, normalSigma, spatialSigma |
| finalProcessor (when injected) | SMAA enabled, threshold, searchSteps; custom processor may provide different metadata |
| light/<node identifier> | type (including Sun), color, intensity, solarRadiusKm, solarDistanceMillionKm, spotAngle/cutoff, castShadows, sourceSize, shadowSamples, affectsReflections |

AO/GI/reflection and temporal objects expose virtual settingsType/settingsInstance;
use both matching addresses when binding replacements, as for denoise/final processors.
Availability/status/tableCapacity/blueNoiseActive and renderer graph selections are
read-only. Requested effect/global-shadow values remain editable and serializable
without RT support; a successful import does not guarantee effective tracing.
Scene light targets remain in the live catalog for explicit callers but are not
graphics preferences and are excluded from graphics import/export.
Scene capacity errors are distinct from unsupported primitives. AO radius and
GI/reflection maxDistance must exceed twice bias; catalog validation checks proposed
pairs before mutation. Unknown target/property/type remains a strict import error.

With no explicit configuration, RendererDeferred starts with
renderer.indirectLighting=2 (GlobalIllumination), globalIllumination.enabled=true
and reflections.enabled=true. These values are requests; inspect availability and
the effective graph on the current device. Explicit renderer configuration or a
user setting can choose IBL/AO or disable either effect. Both examples show
Graphics settings at startup and have reopen buttons; MainLoop does not add it.
Preferences are not restored implicitly.

Lighting debugView appends RayVisibility=11, DiffuseGI=12, ReflectedRadiance=13 and
ReflectionConfidence=14. These inspect resolved post-temporal/post-denoise signals
before receiver BRDF/final color, for the last processed layer. Missing AO gives
one, missing GI/reflection zero. Occlusion=9 includes material AO; Indirect=3 includes
selected GI/reflections. Final color/debug conversion still affects display;
RawHDR bypasses color processing but normalized targets cannot display arbitrary HDR.

Manipulation extends the catalog with lightPose/<node identifier>: position and
orientationDegrees vectors. They are local to the Lights parent and use real
TransformComponent accessors, preserve the other TRS parts, and stop animation
when changed. Dedicated light-property controls also pause animation. These
example-owned target IDs are not a general document/headless scene schema;
other callers must supply explicit extensions/mapping or reject/skip unknown IDs.

Queued changes invalidate engine/SDK histories even after reported partial failure.
Direct stopped-boundary imports/custom mutations must also notify/reset as needed.
See [GraphicsSettingsWindow](../ui/GraphicsSettingsWindow.md),
[histories](TemporalAccumulator.md) and [optional final SMAA](SMAAProcessor.md).

## Serialized document identity

The codec writes schema `bg2e.draw.settings`, version string `1`, the current
`SettingsCatalog::rendererId()` and a components list. The deferred renderer binds
`bg2e.draw.deferred.v1`; the standalone UV tool binds `bg2e.bake.standalone.v1`
with target `bake` and type `bg2e.bake.lightmapSettings.v1`. A custom catalog must
supply its own nonempty identity. Import requires that exact bound identity even
with SkipUnknownOrUnavailable; that policy does not accept schema/type mismatches.
Each component stores id, type and properties; each writable, available graphics
property stores type and value. Scene/diagnostic properties are excluded. Effect type
IDs are `bg2e.draw.rtao.v1`, `bg2e.draw.rtgi.v1`,
`bg2e.draw.reflections.v1`, `bg2e.draw.temporal.v1`, `bg2e.draw.denoise.v1`
and, for the shipped processor, `bg2e.draw.smaa.v1`. Read-only capabilities are not
serialized. Example pose type is `draw.manipulation.lightPose.v1`; unknown example
extensions require an explicit mapping or policy in a different application.

Import checks schema/version/renderer and live component/property types before
mutation. It validates the proposed set and reports setter/rollback failures;
arbitrary setter side effects cannot be made transactionally safe by the codec.
Use typed decimal strings for numeric payloads and SettingsCodec for parse/write.
Preferences stores the codec document as ASCII hex to avoid the legacy JsonNode
writer's escaping limitations; this does not make disk save failures transactional.

## Headless import and precedence

[DefaultOffscreenApplicationDelegate](DefaultOffscreenApplicationDelegate.md) binds
this same catalog directly during the stopped initial preparation. There is no UI
queue. Live defaults are followed by explicitly opted-in persistence only when no
explicit document exists; an explicit graphics document bypasses persistence.
Explicit job internalExtentOverride or scaleOverride applies last. Fixed extent
disables scale; scale enables the live policy and requires a resolution owner.
Output extent, backend and requested scene count remain job values.

Unknown target/property IDs reject unless SkipUnknownOrUnavailable is explicitly
selected; report skipped-property issues from settingsResult(). Applications map
per-run scene IDs with extendSettingsCatalog or choose that skip policy. Import
validates initialized live targets before submitting scene samples. Optional
unavailable effects retain requested values with normal renderer fallback; valid
settings do not imply required baking capability. The job freezes its accepted
configuration and requires a fresh job after changes. See the
[CLI guide](../../apps/draw_offscreen_application.md) for concrete options.

## Graphics settings view

`GraphicsSettingsWindow` shows renderer quality, global shadows, indirect lighting,
AO/GI, reflections, temporal accumulation, denoising, color processing, upscaling
and final processor settings. Show unavailable settings remains available.
Scene environment and light controls belong to the application's environment editor.

Graphics Settings contains no diagnostic or profiling controls. The settings
codec excludes diagnostic properties from serialization and ignores legacy
diagnostic entries on restore. Scene values are likewise excluded from graphics
preference documents. The underlying catalog remains available for programmatic
access to diagnostic properties.
