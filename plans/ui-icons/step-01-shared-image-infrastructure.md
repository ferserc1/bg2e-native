# Step 1 — Shared image infrastructure

Prerequisite: read [summary.md](summary.md) and [progress.md](progress.md). No preceding implementation step.

## Work

1. Inspect `TexturePreview.hpp/.cpp`, `detail/PreviewRegistry.hpp`, `detail/ImGuiBackend.hpp`, `UserInterface.cpp` and all preview consumers. Record existing snapshot, registration and shutdown behavior.
2. Extract private shared registered-image entry/registry types from PreviewEntry/PreviewRegistry. Neutral names such as ImageEntry/ImageRegistry are appropriate. Keep backend adapters and GPU resource types internal; keep public headers free of ImGui.
3. Let the shared registry accept image/sampler owners and retain the existing texture-to-image validation in the UI adapter. Preserve context/device association, sampled color restrictions and layout requirements. Do not make the low-level GPU namespace depend on UI or draw.
4. Extract shared internal image and image-button drawing helpers. These accept the opaque entry, draw dimensions and relevant rendering options; they centralize identifier lookup and ImGui image submission. Do not add a public generic framework or inheritance hierarchy solely to share two small wrappers.
5. Implement existing TexturePreview operations through these primitives. Preserve signatures, integer dimension behavior, retained snapshots across texture reloads, lazy registration, generation handling, invalid-handle behavior and existing consumers.
6. Preserve retirement semantics: entries referenced by submitted draw work remain alive until GPU completion, including when the last public owner disappears during UI composition. Reuse the existing cleanup manager; do not add per-icon/per-frame waitIdle calls. Keep shutdown ordering and outstanding handle invalidation safe.

## Deliverable boundary

Only common resource/drawing extraction and compatible preview adaptation. No icon manager, path cache, widget API or toolbar changes yet. Inspect source/diff for accidental API or ownership changes; no compilation or tests.

## Handoff

Update progress with actual names, ownership and changed files. Report what was extracted and that preview behavior is intended to remain compatible; await user validation. Next: [step 2](step-02-icon-manager.md), adding the permanent path cache on top of these resources.
