# Existing-example advanced effects integration

M03-09 extends the existing deferred and manipulation products. No new target or
application loop is introduced. Compilation and runtime validation belong to the
project lead; source inspection alone does not establish backend coverage.

## Live settings and capabilities

RenderSettingsPreferences binds actual component metadata and matching instance
addresses under stable targets `ambientOcclusion`, `globalIllumination`,
`reflections`, `temporal`, `denoise` and optional `finalProcessor`. AO/GI/reflection
and temporal objects expose virtual `settingsType()`/`settingsInstance()` for
replacement metadata. The default metadata calls live effect accessors; no copied
configuration model is used. Requested settings remain writable and serializable
without hardware support. Read-only availability/status, selected graph fields,
tracing table capacities and blueNoiseActive report effective support separately.
Renderer `indirectLighting` selects IBL/AO/GI independently of reflection settings.
Both examples inject disabled FSR and SMAA: SDK/backend failure uses ordinary
resampling, and SMAA does not require RT or FSR. Resolution remains independently
editable; the manipulation example starts at 70%, with an independent logical
picking resolution. AO/GI/reflection qualities can change their own signal sizes.

Codec imports validate distance > twice bias before writes, including merged
current values. Requested shadow/light settings also persist without queries.
Queued settings invalidate engine/SDK histories after application, including
reported partial failures. Explicit application edits invalidate before potentially
partial material/scene operations. DrawMaterialEditor retains its existing
onChanged callback, including partial edit reports. Existing clone/picking/resource
regressions remain in scope. Direct custom mutation still requires notification.

The renderer builds one shared BlueNoise atlas when a supported advanced effect
has no injected noise owner. Each effect can select hash sampling; per-effect
injected owners take precedence. Optional atlas failure is diagnostic and leaves
hash sampling usable. No atlas is allocated solely for unsupported effects.

Lighting debug views append ray visibility (11), diffuse GI signal (12), reflected
radiance (13), and reflection confidence (14). These display resolved signals after
temporal/denoise, before receiver BRDF and final tone mapping. Absent AO is one;
absent GI/reflection is zero. Existing ambient occlusion includes material AO;
indirect lighting includes GI/reflection when selected. Debug modes describe the
last processed layer. Disable final color processing/use Raw HDR to inspect HDR.

Later M07 correction supersedes the original eight-pair query-shadow policy:
[SoftShadowPass v2](../milestone_07_bg2e_composer/tracing-capacity-16384.md) uses the
exact device-clamped capacity up to 16384 with reserves (2,1,9), and excess slots
use neutral alpha without dropping occluders or disabling whole-scene shadows.
Malformed data remains an error. The original M03 runtime/acceptance record does
not establish native validation of this later correction.

## Optional final processing

See [SMAAProcessor](../../../../doc/api/draw/SMAAProcessor.md). RendererConfiguration
appends shared `finalProcessor`; RendererDeferred exposes it and owns its lifecycle.
Order is tracing -> temporal -> spatial denoise -> lighting/composition ->
reconstruction (resampling/FSR) -> final color -> optional SMAA -> overlays -> UI.
A null component preserves the default graph. Supplied compatible FinalProcessor
objects own their output processing, metadata and descriptors; they may return
false only with original color still usable. Rendering scopes must be closed.

Orthogonal SMAA 1x restores the existing three-pass spatial-AA profile and official
lookup data. It has luma/local-contrast edges, orthogonal area/search weights and
neighborhood blending; no diagonal/corner detection, temporal SMAA, or MSAA.
The public device-level process method also supports no-RT/headless callers and
returns borrowed input when disabled/unavailable, otherwise an owned RGBA16 image.
This is final opaque color AA, not HDR effect filtering or bake accumulation.

## Validation handoff

Use [manual fixtures](../../../../tests/draw/advanced_integration/README.md) and
both existing examples. Record actual hardware/backend, native diagnostics,
capabilities, SDK availability, combinations, edits and retained owner behavior.
Only static checks were performed by the agent. M03-10 must review these concrete
contracts; M03-11 owns cumulative maintained documentation closure.

## M03-09 correction: visible settings wiring

Lead screenshots showed only the scene windows. Source inspection confirmed the
initial delivery registered effect metadata but did not create, connect or draw
GraphicsSettingsWindow in either example. Earlier claims of visible controls were
incorrect. Both UI delegates now own a shared preferences facade, connect it before
the first renderer attachment, bind a graphics window, show it at startup and draw
it every UI frame. A scene-controls button reopens it. Borrowed PreferencesStore
scopes are separate per example; save/restore is explicit. Existing weak lifecycle
callbacks rebind catalog generations on renderer changes. Manipulation cleanup
clears its window/facade before renderer teardown.

The same inspection found finalProcessor recording in both update and record.
The update call is removed: optional SMAA now executes only after the current final
color, once per acquired slot/serial, before overlays. This prevents stale color
processing and duplicate-serial failure when SMAA is enabled. Static checks only;
lead compilation and startup/reopen/toggle/resize/SMAA checks remain pending.
