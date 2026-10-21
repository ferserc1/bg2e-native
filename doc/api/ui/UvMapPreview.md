# UvMapPreview retirement

The former UvMapPreview class/header is removed. Use [BakeLightmapPreview and bake::UvMapPreviewRenderer](../bake/UvMapPreviewRenderer.md).
Keep the Engine/device alive until preview snapshots and UI registrations are
released. UI headers expose no ImGui types; native backend implementations live inside GPU.
