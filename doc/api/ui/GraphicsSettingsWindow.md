# GraphicsSettingsWindow

Header: `<bg2e/ui/GraphicsSettingsWindow.hpp>`. Namespace: `bg2e::ui`.
The window owns a shared draw::RenderSettingsPreferences facade; persistence is
optional and borrowed by that facade. This is separate from production
RenderSettingsWindow. No imgui type is exposed in the public contract.

Construct/retain the facade, connect(delegate,extensions) before the delegate's
first renderer binding, then bind(facade). Call draw() in drawUI(); the window is
not automatically registered by MainLoop. open()/close() manage visibility;
provide an application button to reopen it. The two shipped examples do this.

```cpp
// UI delegate members; keep them alive while the window is displayed.
std::shared_ptr<bg2e::draw::RenderSettingsPreferences> preferences;
bg2e::ui::GraphicsSettingsWindow graphics;

// During UI object construction, before first renderer attachment.
preferences = std::make_shared<bg2e::draw::RenderSettingsPreferences>();
preferences->connect(sceneDelegate); // shared DefaultRenderLoopDelegate owner.
graphics.bind(preferences);
graphics.open();

// Inside the existing UI frame callback.
graphics.draw();
```

The snippet omits surrounding class/method bodies and optional persistence;
sceneDelegate must already exist and targets must not be bound yet. Existing
attach/detach callback owners must be explicitly composed. Extensions receive
actual Renderer and SettingsCatalog after standard target binding. Supply live
metadata, matching instance addresses and retained owners, not settings copies.

Sections enumerate fresh graphics descriptors/getters each frame. Scene
environment/lights and debug/capability descriptors are filtered out. Global
shadows, indirect mode, AO/GI, reflections, temporal accumulation, denoising, color,
resolution/upscaling and final processing remain graphics controls.
Unavailable graphics descriptors are hidden. queueEdit, importFile and restore queue generation-checked changes;
pending() disables overlapping edits/export/persist. lastResult() reports failures.
Renderer replacement rebinds generations and invalidates stale handles/editor IDs.

Import/export use strict SettingsCodec JSON. persist()/restore() require an explicit
borrowed Preferences service; nothing restores automatically. clear() removes the
facade reference and invalidates weak completion callbacks. Clear borrowed targets
before destroying instances; keep Engine/Device alive until GPU owners are released.
Manipulation invokes its existing UI cleanup before renderer teardown.

See [live settings and target IDs](../draw/Settings.md),
[deferred tutorial](../../tutorials/draw/10_deferred_renderer.md) and
[light/transform editing](../../apps/draw_manipulation.md).

Graphics Settings contains no profiling controls. The draw composer exposes
profiling separately through Window → Performance.

The final **Appearance: Bloom and color correction** section exposes **Bloom**,
followed by **Color**:
Enabled, Intensity, Threshold (max RGB, linear), Scatter, Pyramid levels, and Initial
resolution (Half/Quarter). These controls use the live settings catalog and the
existing JSON preference import/export path. Defaults are enabled, 0.08 intensity,
1.0 threshold, 0.7 scatter, six levels and Quarter resolution. Bloom-only edits on
the standard renderer preserve temporal/denoiser histories and are queued as CPU
value updates; pyramid changes retire safely per acquired frame slot. Unsupported
bloom controls are hidden; availability/status remain diagnostics.

Quality profiles are defined entirely by `graphicsQualityPresets` in the window
header. Each entry supplies its name, preferred supported upscaling methods
(MetalFX, then FSR), and exact typed values for every quality setting, including
hidden fields. Add or edit entries in this array to change the presets; drawing
and application enumerate the array without depending on preset indices.
Bloom and color correction are deliberately absent from the profiles.

The quality selector follows file import/export and acts only when selected.
It can reapply the same profile after manual edits or file import. Profile edits
use one validated, generation-checked import transaction through the existing
facade; no renderer setter runs during drawing. The selector does not reset
settings on window creation, bind, or subsequent frames. If neither MetalFX nor
FSR is supported, application reports the failure without applying the profile.

AO and GI controls appear only for their selected indirect lighting mode.
Selecting either mode also enables that effect. Their enable toggles and all
blue-noise toggles are hidden; profiles explicitly enable blue noise. The
**Quality (render resolution)** selectors control each individual effect's
resolution independently of the overall quality profile.

Temporal and Denoise each have their own **Show advanced** checkbox, initially
off. Temporal normally exposes Enabled, Freeze at limit, Max frames, and Adaptive
enabled; Max frames is editable only with Freeze at limit. Denoise normally
exposes Enabled and Kernel radius. Advanced fields use bounded sliders. Count
controls and GI/reflection distances use integer sliders, while continuous
controls retain floating-point values. Ray biases display four decimal places.
Sliders support Ctrl+click keyboard entry and clamp edits to the UI ranges.
Bloom pyramid levels offer 4, 5, 6 (default), 7, and 8; an imported value outside
that list remains visible as the current value until explicitly changed.
