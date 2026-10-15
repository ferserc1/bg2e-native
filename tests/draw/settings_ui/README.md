# M01-14 graphics settings UI checks

The project lead compiles bg2e and the existing production/draw hosts. No target,
CMake change or new executable is introduced. These are manual checks; the
implementing agent has not compiled or executed them.

## Host integration

Use the existing deferred fixture/next M01-15 example with shared objects:

```cpp
auto preferences = std::make_shared<bg2e::draw::RenderSettingsPreferences>(&appPreferences);
preferences->connect(delegate); // Before the delegate's first renderer binding.
bg2e::ui::GraphicsSettingsWindow window;
window.bind(preferences);
// Call window.draw() from the normal UserInterfaceDelegate draw callback.
```

Preferences storage is optional/borrowed and must outlive the facade. Existing
lifecycle callback owners compose bind/clear explicitly rather than calling
connect twice. The window never owns/replaces those callbacks. Destroy/clear
window before UI shutdown; pending changes may still apply to the renderer,
but weak completion callbacks must not touch the destroyed window.

## Lead checks

1. Open before renderer initialization: waiting message, no dereferences/errors.
   Bind a populated catalog: sections group components/categories; control values
   match C++ getters. Test FinalLayer, shadows, environment resource, lights,
   scale and selected bilinear/FSR/custom upscaler. Existing RenderSettingsWindow
   must continue working with production rendering.
2. Edit exposure, color, vectors, enums and scale. Values change at the next safe
   prepare hook; pending editors are disabled. Validate range/cross-property
   failures (e.g. shadow bias/distance) are displayed, with original live values
   retained. No direct target setter or resource rebuild occurs during UI draw.
3. Resize/replace renderer or upscaler while an edit/import is queued. Stale
   generation diagnostics are displayed; sections/editor IDs refresh and no
   dead object is read. Change light type/availability without structural rebind:
   availability refreshes every UI frame. Hidden unavailable controls can be
   shown disabled; implemented status/capability readouts remain readable.
4. File Import uses strict SettingsCodec and queued atomic validation/application;
   Export reads current getters. Round-trip paths with Unicode/quotes and numeric
   extremes. Invalid JSON, unavailable/unknown settings, cross-validation and
   filesystem errors produce diagnostics. Strict Reject is the UI default;
   programmatic importFile accepts explicit SkipUnknownOrUnavailable.
5. Save preferences reads the facade's getters; Restore preferences queues edits
   before acquisition. Without a Preferences service, both controls are disabled.
   Programmatic persist/restore report missing-service failures. Explicit file
   import does not merge persisted defaults. No implicit restore/save on draw,
   bind or destruction. Persistence still has the legacy service's log-only
   durability reporting; a successful facade call is not proof of disk durability.
6. Supply custom UInt at UINT32_MAX, Double values differing below float precision,
   sparse int64 enums above 2^53, long strings/paths, bool false and readonly
   properties. UInt uses exact decimal text, Double uses its native double input,
   enum selections retain their int64 identities, and strings size buffers from
   actual content. Unsupported object-valued settings are explained, not traversed
   by raw pointer. Readonly controls never invoke original setters.
7. Resource-picker metadata for HDR environment source preserves String/Path
   storage and filters. Browse/Clear returns transient intent, realization occurs
   later via queue and RendererDeferred prepare. Invalid HDR retains the previous
   environment and reports the renderer's error. No thumbnail GPU upload or
   legacy TextureWidgets is needed for the reference deferred example's settings;
   full material/preview/editor adaptation remains M02/M06.
8. Clear/rebind/destroy the window before completion and while requests are pending:
   no callback accesses a raw window pointer and a newly bound facade receives no
   old result. Two windows have independent native editor IDs and view state.

No build/runtime/shader tests were run by the agent. Static checks cover public
native-free interfaces, local includes, signatures/call sites, transient proxy
accessors, weak callback ownership, safe queue flow, links and whitespace.
