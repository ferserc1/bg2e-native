# Step 07: Delete Old App-Local Files

## Files to Delete

- `apps/model_edit/src/UIRenderSettingsWindow.hpp`
- `apps/model_edit/src/UIRenderSettingsWindow.cpp`
- `apps/bg2e_composer/src/UIRenderSettingsWindow.hpp`
- `apps/bg2e_composer/src/UIRenderSettingsWindow.cpp`

## Notes

- No CMakeLists.txt changes needed — the project uses glob patterns (`src/**/*.cpp`) for source file discovery
- Both apps now use the engine-level `bg2e::ui::UIRenderSettingsWindow` from `lib/include/bg2e/ui/UIRenderSettingsWindow.hpp`
- The old app-local versions had a dependency on `AppDelegate`; the new engine version depends only on `RendererDeferred` and `RenderSettingsPreferences`
