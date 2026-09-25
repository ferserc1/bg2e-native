# Plan Status

## Step 1 completed: FileHistory singleton
Date: 2026-09-25
Changes:
- `lib/include/bg2e/app/FileHistory.hpp`: added the singleton API, known file types, type registry, MRU history operations, and clearing methods.
- `lib/src/bg2e/app/FileHistory.cpp`: implemented automatic registration of image, BG2 model, and 3D model types plus bounded in-memory histories.
- `lib/include/bg2e/app/all.hpp`: exported `FileHistory` through the aggregate app header.

## Step 2 completed: FileHistoryWidget
Date: 2026-09-25
Changes:
- `lib/include/bg2e/ui/FileHistoryWidget.hpp`: added the reusable file-history picker API with text/image triggers and optional thumbnail provider.
- `lib/src/bg2e/ui/FileHistoryWidget.cpp`: implemented empty-history dialog fallback, ImGui popup history list, fixed "Open file..." action, MRU commits, and path tooltips.
- `lib/include/bg2e/ui/all.hpp`: exported `FileHistoryWidget` through the aggregate UI header.
- Correction (2026-09-25): removed `imgui.h` from `FileHistoryWidget.hpp`; the public API now uses the opaque `FileHistoryWidget::TextureID` (`uint64_t`, compatible with `ImTextureID`/`VkDescriptorSet`), with ImGui types confined to the `.cpp`. Plan docs (`summary.md`, `step-02`, `step-03`) updated with the "no imgui.h in engine headers" rule.

## Step 3 completed: TextureWidgets and MaterialEditor integration
Date: 2026-09-25
Changes:
- `lib/include/bg2e/ui/TextureWidgets.hpp`: added engine initialization, the image file-history picker, thumbnail descriptor cache, and opaque thumbnail API types.
- `lib/src/bg2e/ui/TextureWidgets.cpp`: routed texture selection through `FileHistoryWidget`, added `TextureCache` thumbnail loading, graceful load failure handling, and Vulkan descriptor cleanup.
- `lib/include/bg2e/ui/MaterialEditor.hpp`: added stack-object engine initialization and stored the engine pointer.
- `lib/src/bg2e/ui/MaterialEditor.cpp`: forwards the engine to all six texture widgets and removed the unused direct file-dialog include.

## Step 4 completed: Application wiring
Date: 2026-09-25
Changes:
- `apps/model_edit/src/SubmeshWindow.cpp`: initializes `MaterialEditor` with `delegate->engine()`.
- `apps/bg2e_composer/src/SubmeshWindow.cpp`: initializes `MaterialEditor` with `delegate->engine()`.
