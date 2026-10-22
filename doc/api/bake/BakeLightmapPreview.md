# BakeLightmapPreview

`<bg2e/ui/BakeLightmapPreview.hpp>` exposes a small UI adapter for a completed
`bake::LightmapResult`. It creates a `draw::Texture` over the result's existing
GPU image; it does not copy or read the pixels back to the CPU. The UI session
registers a retained `TexturePreview`, so the GPU image remains alive while UI
draw work can reference it.

Initialize `ui::UserInterface` with the same `draw::Engine` that owns the bake
image. Obtain the current optional `baker.result()` and, only when present, pass
its retained value to `setResult(ui, engine, result)` on the UI thread. The
result must have a nonzero extent matching its image and the image must be in
shader-read-only layout on that device. `draw(width, height)` displays it.
Calling `setResult` again retires the previous preview before binding the new
image, which is the required path after a bake reset, target replacement or
resolution change. `clear()` releases the session preview before its texture
owner; call it before UI and engine teardown. The widget does not own the
`UserInterface` or `Engine`.

This widget shows the baked multiplier texture. For mesh UV coverage, per-submesh
wireframes and atlas boundaries, use `bake::UvMapPreviewRenderer`, which supports
UV1 and UV2, does not mutate the source CPU mesh, and produces a sampled image
through `gpu::Device`. The diagnostic is rasterized on CPU and uploaded as RGBA8;
it has no camera-renderer or backend-specific draw dependency. Bind its
`image()` through `BakeLightmapPreview::setImage` to display it in the same draw
UI session. The former `ui::UvMapPreview` has been removed; use the bake diagnostic
and this UI bridge. No ImGui or native graphics types are exposed by these APIs.


## Public methods

| Method | Contract |
|---|---|
| setResult(UserInterface&, draw::Engine&, const bake::LightmapResult&) | Validate matching nonzero extent, then register its retained image. Obtain the optional current result before dereferencing it. |
| setImage(UserInterface&, draw::Engine&, std::shared_ptr<gpu::Image>) | Bind a UV diagnostic or finished image on the same device in ShaderReadOnly layout. Uses linear or nearest filtering according to native capabilities. |
| draw(uint32_t width, uint32_t height) | Draw only while the session preview is valid. |
| valid() const | Reports whether the registered session entry remains usable. |
| clear() | Release preview registration before the retained texture; required before UI/Engine teardown. |

Replacement registers a new image; raw image pointers are not sufficient owners.
This API exposes no native descriptor or ImGui type. See the
[UI integration](../ui/TexturePreview.md) and [tutorial](../../tutorials/draw/12_lightmaps.md).
