# M01-14 actual graphics settings UI

[Step](step_14_settings-ui.md) · [Progress](../progress.md) · [Lead checks](../../../../tests/draw/settings_ui/README.md)

`ui::GraphicsSettingsWindow` is the new draw settings view. The production
`RenderSettingsWindow` and its option model remain intact. Bind a shared
`draw::RenderSettingsPreferences` after connecting the facade to the delegate
before first renderer binding; call draw from the normal UI callback. The window
owns only the facade/view state, never a renderer pointer or settings value model.
Clear/bind discards prior view state; queued completion callbacks use weak state
and cannot call a destroyed/rebound window. Lifecycle ownership stays with the
facade/application rather than being replaced by the widget.

Each frame enumerates catalog.properties(showUnavailable), groups component IDs
and metadata categories, reads current values through generation-bearing handles
and edits transient values. Availability refreshes even when the generation has
not changed. Generation changes rebuild the descriptor view/native editor scope;
no old target pointers/metadata values survive. Pending edits disable editors.
Unavailable methods can be hidden or shown disabled with an explanation; real
status/capability properties remain visible. No unsupported RT effects are added.

`ReflectionWidget::editValue(any&, PropertyInfo, editable)` reuses the scalar
reflection editors but replaces both accessors with transient value proxies.
It never invokes the descriptor's original getter/setter or follows object
pointers. Exact UInt decimal input and native Double input avoid signed-int/float
narrowing. Path and arbitrary-length String inputs preserve storage types and
allocate sufficient edit capacity. Sparse enums keep int64 identities. Resource
metadata reuses ResourcePicker's native-free file intent and String/Path storage.
Legacy drawProperties/drawActions behavior remains unchanged.

QueueEdit delegates to queueSet; Import uses SettingsCodec::readFile + queueImport,
strict Reject by default. File dialogs use existing app::FileDialog wrappers.
Export reads live catalog getters through SettingsCodec::exportFile. Native-free
programmatic importFile/exportFile/persist/restore methods expose the same behavior;
explicit SkipUnknownOrUnavailable is available programmatically. Exceptions and
completion validation/rollback/stale-generation issues are displayed in the panel.
No settings writes, uploads or graph rebuilds occur directly in UI draw.

Optional persistence controls call the existing facade. New hasPersistence and
queueRestore allow a disabled-state UI and safe restoration respectively.
queueRestore reads/decodes the saved typed document then queues the same validated
application path; absent storage/data does not invent defaults. Save/export are
blocked while this window has pending edits so they do not silently save an
older value. No implicit save/restore occurs on bind/draw/destruction. The borrowed
Preferences service must outlive the facade; its existing log-only durability
reporting is unchanged.

The reference example 17 uses sky/material controls but no TextureWidgets image
preview. This step needs ResourcePicker for environment HDR intent only: it does
not instantiate the legacy Vulkan TextureWidgets, upload thumbnails during UI
recording, or add a second preview registration system. Full draw material and
preview/picking widgets remain M02/M06. New public window/editor interfaces expose
no ImGui/Vulkan types; all widget rendering uses ui wrappers.

Baseline b955833f314d plus preserved earlier uncommitted work/lead fixes. M01-13
accepted by project lead continuation instruction, backend coverage unspecified.
No CMake/production renderer changes, configuration, C++/shader compilation or
runtime execution by the implementing agent. Static ownership/queue/proxy/type/
include/signature/native-boundary/link/whitespace checks and the linked lead
checks form the handoff. M01-15 integrates the window with the actual example;
M03 registers new implemented effects, M02/M06/M07 adapt remaining widgets
independently instead of rewriting this settings window.
