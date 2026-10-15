# M01-15 actual deferred example

[Step](step_15_working-example.md) · [Progress](../progress.md) · [Lead acceptance checks](../../../../examples/draw/10_deferred_renderer/README.md)

## Product and integration

`examples/draw/10_deferred_renderer`, target `draw_deferred_renderer`, is registered
by examples/CMakeLists.txt and a local bundle_app_sdl invocation, following the
step's explicit target/bundle/assets deliverable. No general CMake helper,
production target or shader layout was changed. Existing engine shader/asset
bundling supplies the paired draw PBR/environment/query/upscale shaders and
assets; no app-private renderer or execution loop is added.

MainLoop's EngineConfig overload selects native Metal/macOS or Vulkan/other
platforms, with optional explicit backend and validation flags. SceneDelegate
extends the non-template DefaultRenderLoopDelegate; independent UiDelegate draws
GraphicsSettingsWindow and scene controls through ui wrappers. No new code uses
native API/ImGui handles or production rendering calls.

The scene contains the provided two-submesh OBJ and ten explicit material maps,
nested original/independent tinted clone, ordinary glass and solid glass clones,
floor, alpha-checker canopy and a packed-channel/UV1 cube. Four deterministic
rotating omni lights respect the eight-light ABI; the old reference used ten.
A scene CameraComponent with explicit perspectiveRH_ZO projection and orbit
parent supplies finite zero-to-one camera data independent of GLM include order.
Camera/light animation uses sceneDeltaSeconds, preserving paused/retained timing.

The clone initialization guard inspects distinct CPU/GPU geometry, material,
texture, image and sampler objects and independent tint values. Live clone tint/
roughness, glass alpha/visibility, cutout threshold/visibility and packed metalness
channel/inversion edits queue through renderer generations at pre-acquire GPU
boundaries and reset SDK history. Geometry/texture updates are not required for
these scalar edits. Packed R=51/204, G=166, B=230, A=255 enables exact debug
channel expectations; metalness uses UV1=2*UV0 and additional (2,1) scale.

## Settings, extension and replacement

Live ResolutionSettings starts enabled at .67. Bilinear, FSR and an application
resampler are choices in this application's menu, not an engine enum. The latter
subclasses the public ResamplingUpscaler with a stable custom identifier, live
bypass and record count metadata. The public CountingGBuffer is genuinely
injected and reports per-stage calls. FinalLayer is injected/reused with actual
reconstructed-color contract and live settings.

Integration correction: FinalLayer now accepts optional final constructor
argument `bool reconstructedColor=false`, matching finalContract. Set true when
injecting its output-sized-HDR contract into RendererDeferred. The constructor
must declare its real contract before Renderer::validateConfiguration runs;
initialization alone is too late. Existing standalone constructors/default
internal-color contract remain source-compatible; rebuild library/callers.

Renderer replacements are queued setRenderer calls, preserving the scene plus
shared final, shadow, resolution and counter objects. Cleanup releases their
native resources before the next renderer initializes them. The UI stale-edit
probe queues an old final command then replacement; delegate generations reject
the delayed command. Material/scene callbacks capture weak State and are checked
against the renderer generation. Cleanup drains/render cleanup, clears catalog
owners, explicitly unloads all retained drawables and releases scene references
before engine destruction. Pending callbacks never capture a raw UI window.

A shared RenderSettingsPreferences connects before first renderer binding and
borrows application-owned Preferences. GraphicsSettingsWindow provides actual
file/persistence/edit/availability diagnostics. Startup flags --settings=FILE and
--restore-preferences queue one restore/import after initial binding; explicit
file wins. Startup validation issues reach the scene status; invalid file IO/JSON
throws to the main error boundary. Nothing restores settings implicitly. Files
must match the chosen upscaler under the strict codec policy.

--no-rt prevents Drawable tracing realizations and disables light shadow intent
and query settings; it does not falsify device capabilities or disable native
extensions. Genuine no-query hardware follows SoftShadowPass availability/PBR
fallback. Optional FSR failure similarly preserves reduced bilinear output.
Native Metal upscaling, object/reactive motion and advanced engine temporal
processing remain unimplemented, explicitly described in the example checklist.

## Validation and downstream boundary

The lead corrected M01-14 by adding virtual ~Window() = default to the polymorphic
base; source inspection confirms it is retained. M01-14 accepted by lead
continuation; compile/runtime/backend coverage beyond that report is unspecified.
Baseline b955833f314d plus all existing uncommitted work/lead changes preserved.

Agent validation is static only: local includes/declarations/signatures and
virtual dispatch, real assets, constructor contracts, shared/weak ownership,
cleanup ordering, CLI option paths, queue/reset/slot usage, CMake target/resource
registration, native dependency boundary, documentation links and whitespace.
No configuration, C++/shader compilation, runtime execution or image comparison
was performed. Runtime-looking assertions/counters are implemented probes, not
agent-observed results. The linked checklist belongs to the project lead.

M01-16 must review actual code against every pending milestone and distinguish
lead acceptance from unspecified platform evidence. M02/M04/M05 may derive their
products from this scene, and M03 extends this target instead of inventing an
additional advanced-rendering example. The detailed reference capability
checklist keeps selection/gizmos, advanced RT/history/denoise/spatial AA, bake and
offscreen work scheduled without placeholder controls or claimed completion.

## Lead-reported cutout-table overflow correction

The lead reported startup exit from the eight-pair alpha table guard. Generic
geo::createCube produces six material submeshes, while the fixture configured
only material(0); unused faces retained positive default cutoff and independent
white fallback realizations. The example now combines the full index range into
one submesh for each uniform-material canopy/channel cube and explicitly sets
opaque floor/channel cutoff to zero. All vertices/indices/faces remain; canopy
alpha testing applies to every face. The default scene now requires one cutout
pair. No GPU/shader capacity, general cube primitive, clone semantics or shadow
occluder filtering was changed. Lead rebuild/runtime confirmation is pending;
the earlier review acceptance gate is held until this correction is validated.
