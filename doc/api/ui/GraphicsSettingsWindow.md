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
**Show unavailable settings** controls unavailable graphics descriptors. queueEdit, importFile and restore queue generation-checked changes;
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
