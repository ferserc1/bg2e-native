# render-settings-preferences

## Problem Statement

The bg2 engine production applications (model_edit and bg2e_composer) save and restore render settings through duplicated `saveSettings()`/`restoreSettings()` methods in each app's `AppDelegate`. This approach has several issues:

1. **Duplicated code** — both apps have nearly identical ~120-line save/restore methods
2. **Incomplete persistence** — RenderScale index, IndirectLightingMode, and all RTGI settings are not saved
3. **Immediate disk writes** — `saveSettings()` writes to disk on every slider drag
4. **No centralized management** — settings logic scattered across AppDelegate, UIRenderSettingsWindow, and ToolBar

## Proposed Solution

Introduce three new components:

1. **`bg2e::base::Timeout`** — A timer manager integrated into `MainLoop` that supports delayed callbacks with an `executeOnExit` flag for guaranteed persistence on app shutdown
2. **`bg2e::render::RenderSettingsPreferences`** — A thin wrapper over `RendererDeferred` that provides typed accessors for all render settings and persists them via `bg2e::app::Preferences`
3. **`bg2e::ui::UIRenderSettingsWindow`** — Refactored from app-local to engine-level, shared by both apps

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      MainLoop                           │
│  ┌───────────────────────────────────────────────────┐  │
│  │                  base::Timeout                    │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │ Timer { callback, 60s, executeOnExit=true } │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  │  executeTimers()     ← called each frame          │  │
│  │  executeExitTimers() ← called at loop exit        │  │
│  └───────────────────────────────────────────────────┘  │
│       │                          │                      │
│       ▼                          ▼                      │
│  ┌──────────┐          ┌──────────────────────┐         │
│  │ Renderer │◄─────────►│ RenderSettingsPrefs   │         │
│  │ Deferred │  read/   │  _dirty flag          │         │
│  │          │  write   │  _prefs (scope:"render")│       │
│  └──────────┘          │  persist() ← timer    │         │
│       ▲                │  load()    ← init     │         │
│       │                └──────────────────────┘         │
│       │                         ▲                       │
│  ┌────┴─────────────────────────┴──────────────────┐    │
│  │           UIRenderSettingsWindow                 │    │
│  │  reads: renderer->xxx()                         │    │
│  │  writes: prefs->setXxx() → renderer->setXxx()   │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘

App (model_edit / bg2e_composer)
  AppDelegate::initWorkspace()
    1. Create RenderSettingsPreferences(renderer)
    2. prefs->load()          ← restore saved settings
    3. Register timer: 60s, executeOnExit=true
    4. Init UIRenderSettingsWindow(renderer, prefs)
```

## Data Flow

```
User changes slider in UIRenderSettingsWindow
  → prefs->setXxx(value)
    → renderer->setXxx(value)   [immediate effect]
    → _dirty = true             [mark for persistence]
  → (up to 60s later) timer fires
    → prefs->persist()
      → reads all values from renderer
      → writes to preferences_render.json
      → _dirty = false

App exit
  → MainLoop calls executeExitTimers()
    → timer callback fires (regardless of elapsed time)
    → prefs->persist()          [guaranteed save]
  → resource cleanup proceeds
```

## File Table

| Action | File | Description |
|--------|------|-------------|
| Create | `lib/include/bg2e/base/Timeout.hpp` | Timer manager header |
| Create | `lib/src/bg2e/base/Timeout.cpp` | Timer manager implementation |
| Create | `lib/include/bg2e/render/RenderSettingsPreferences.hpp` | Preferences wrapper header |
| Create | `lib/src/bg2e/render/RenderSettingsPreferences.cpp` | Preferences wrapper implementation |
| Create | `lib/include/bg2e/ui/UIRenderSettingsWindow.hpp` | Shared render settings window header |
| Create | `lib/src/bg2e/ui/UIRenderSettingsWindow.cpp` | Shared render settings window implementation |
| Modify | `lib/include/bg2e/base/all.hpp` | Add Timeout include |
| Modify | `lib/include/bg2e/render/all.hpp` | Add RenderSettingsPreferences include |
| Modify | `lib/include/bg2e/ui/all.hpp` | Add UIRenderSettingsWindow include |
| Modify | `lib/include/bg2e/app/MainLoop.hpp` | Add Timeout member and accessor |
| Modify | `lib/src/bg2e/app/MainLoop.cpp` | Integrate executeTimers/executeExitTimers |
| Modify | `apps/model_edit/src/AppDelegate.hpp` | Add RenderSettingsPreferences, remove old methods |
| Modify | `apps/model_edit/src/AppDelegate.cpp` | Replace save/restore with new system |
| Modify | `apps/bg2e_composer/src/AppDelegate.hpp` | Same as model_edit |
| Modify | `apps/bg2e_composer/src/AppDelegate.cpp` | Same as model_edit |
| Delete | `apps/model_edit/src/UIRenderSettingsWindow.hpp` | Replaced by engine version |
| Delete | `apps/model_edit/src/UIRenderSettingsWindow.cpp` | Replaced by engine version |
| Delete | `apps/bg2e_composer/src/UIRenderSettingsWindow.hpp` | Replaced by engine version |
| Delete | `apps/bg2e_composer/src/UIRenderSettingsWindow.cpp` | Replaced by engine version |

## Steps

1. [bg2e::base::Timeout — Timer Manager](step-01-timeout-manager.md)
2. [Integrate Timeout into MainLoop](step-02-integrate-timeout-mainloop.md)
3. [bg2e::render::RenderSettingsPreferences](step-03-render-settings-preferences.md)
4. [Move UIRenderSettingsWindow to bg2e::ui](step-04-move-ui-render-settings-window.md)
5. [Integrate in model_edit](step-05-integrate-model-edit.md)
6. [Integrate in bg2e_composer](step-06-integrate-bg2e-composer.md)
7. [Delete old app-local files](step-07-delete-old-files.md)

## Thread Safety

- `Timeout::executeTimers()` and `executeExitTimers()` are called from the main thread only. No mutex needed since the main loop is single-threaded.
- `bg2e::app::Preferences` is accessed from the main thread only (both `load()` and `persist()`).

## Notes

- **Preferences scoping:** Using scope `"render"` creates `preferences_render.json`, separate from the global `preferences.json` (which stores window size etc.)
- **Exit guarantee:** `executeOnExit = true` ensures `persist()` fires even if the user changes a setting and immediately closes the app. The callback always fires during `executeExitTimers()` regardless of elapsed time.
- **No `markDirty()` API needed:** Every setter on `RenderSettingsPreferences` sets `_dirty = true` internally. The UI window just calls setters.
- **`persist()` idempotency:** If `_dirty` is false, `persist()` returns immediately with no disk I/O. Safe to call at any frequency.
- **Settings managed (complete list):**

| Category | Setting | Preferences key |
|----------|---------|-----------------|
| RenderScale | scale option index | `render_scale_optionIndex` |
| IndirectLighting | mode (0=RTAO, 1=RTGI) | `render_il_mode` |
| RTAO | qualityIndex | `render_ao_qualityIndex` |
| RTAO | sampleCount | `render_ao_sampleCount` |
| RTAO | bounceCount | `render_ao_bounceCount` |
| RTAO | radius | `render_ao_radius` |
| RTAO | bias | `render_ao_bias` |
| RTAO | falloff | `render_ao_falloff` |
| RTAO | bounceAttenuation | `render_ao_bounceAttenuation` |
| RTGI | enabled | `render_gi_enabled` |
| RTGI | qualityIndex | `render_gi_qualityIndex` |
| RTGI | sampleCount | `render_gi_sampleCount` |
| RTGI | bounceCount | `render_gi_bounceCount` |
| RTGI | rayBias | `render_gi_rayBias` |
| RTGI | maxDistance | `render_gi_maxDistance` |
| RTReflections | enabled | `render_reflect_enabled` |
| RTReflections | sampleCount | `render_reflect_sampleCount` |
| RTReflections | maxRoughness | `render_reflect_maxRoughness` |
| RTReflections | rayBias | `render_reflect_rayBias` |
| RTReflections | maxDistance | `render_reflect_maxDistance` |
| RTReflections | roughnessSpread | `render_reflect_roughnessSpread` |
| Temporal | mode | `render_ta_mode` |
| Temporal | historyWeight | `render_ta_historyWeight` |
| Temporal | depthThreshold | `render_ta_depthThreshold` |
| Temporal | normalThreshold | `render_ta_normalThreshold` |
| Denoise | kernelRadius | `render_denoise_kernRadius` |
| Denoise | depthThreshold | `render_denoise_depthThreshold` |
| Denoise | normalThreshold | `render_denoise_normalThreshold` |
| Denoise | depthSigma | `render_denoise_depthSigma` |
| Denoise | normalSigma | `render_denoise_normalSigma` |
