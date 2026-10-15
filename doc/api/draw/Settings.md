# Live graphics settings and upscaling

Headers: `<bg2e/draw/SettingsCatalog.hpp>`, `<bg2e/draw/SettingsCodec.hpp>`,
`<bg2e/draw/RenderSettingsPreferences.hpp>`, `<bg2e/draw/Upscaler.hpp>`,
`<bg2e/draw/FsrUpscaler.hpp>`, `<bg2e/ui/GraphicsSettingsWindow.hpp>`.

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

`RenderSettingsPreferences` is a shared stateless facade. Optional
`app::Preferences*` persistence is borrowed. `connect(shared_delegate, extensions)`
installs weak attach/detach callbacks before first renderer binding. `queueSet`,
`queueImport` and `queueRestore` commit through the delegate's pre-acquisition
safe boundary and reject stale renderer/catalog generations. Direct bind/apply/
restore require a stopped boundary. No values load implicitly: explicit settings
documents bypass stored defaults; absent properties retain current live values.
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
proxies without following raw object pointers. Production RenderSettingsWindow
remains separate. The minimal HDR picker is ResourcePicker; this delivery does
not adapt all production material/texture/picking widgets.

## Resolution and optional reconstruction

`ResolutionSettings` is authoritative on Vulkan and Metal. Enabled policy uses
floor(output × scale), with scale in [0.25, 1]; disabling restores the original
fixed/follow-output extent. Default settings are disabled/1.0; the deferred
example explicitly enables 0.67. Reduction remains usable without FSR.

`Upscaler` is a public injectable lifecycle with identifier/label, requirements,
availability/status, prepare/record/reset and reflected settings metadata.
`ResamplingUpscaler` supplies bilinear HDR reconstruction. A false record result
falls back to ordinary resampling. Output is full-resolution linear RGBA16F HDR
before the single final color operation. Debug sources remain internal-sized.

`FsrUpscaler` is optional Vulkan SDK integration; native SDK code is isolated in
`gpu::vk::FidelityFXBridge`. Metal, missing SDK and initialization failures report
status and retain bilinear rendering. Primitive device capabilities do not prove
SDK availability. At most four SDK queued frames are supported. Acquired-slot
resources and ordered command recording avoid routine per-frame idle/readback.
Motion is camera-only with preserved opaque depth and conventional finite RH
perspective/zero-to-one depth. Identity/orthographic cameras use unjittered
fallback. Moving objects/glass can ghost; no reactive masks or general engine
histories are supplied. Attach/resize/replacement/projection/discontinuity and
explicit `RendererDeferred::resetUpscaler()` camera cuts reset history.
Native Metal upscaling and DLSS are not implemented.

See [working tutorial](../../tutorials/draw/10_deferred_renderer.md).
