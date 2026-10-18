# UvMapPreviewRenderer

`<bg2e/bake/UvMapPreviewRenderer.hpp>` provides a renderer-independent UV
diagnostic. Construct it with an initialized `gpu::Device` and a resolution in
`[1, 16384]`. `render(mesh, uvSet)` accepts `0` for `texCoord0` / UV1 or `1`
for `texCoord1` / UV2. It draws deterministic submesh colors, triangle
wireframes and the atlas border. Invalid indices and non-finite UV triangles are
skipped so malformed input can still be inspected. The source mesh is read-only
and is never rebuilt or modified.

The diagnostic rasterizes on the CPU, then uploads an RGBA8 sampled image using
the backend-neutral `gpu::Image` API. The returned shared image remains owned by
the caller when the renderer changes resolution or is destroyed. A resolution
change drains device work and replaces the renderer's target; previously
retained image owners remain valid. The image uses the conventional top row for
UV `v = 0`. `pixels()` exposes the latest tightly packed RGBA8 CPU buffer for
diagnostics.

To display `image()` in a draw-backed UI, pass it to
`ui::BakeLightmapPreview::setImage`. That widget and the UI session must be
cleared before their device is destroyed. The preview does not require the
production renderer or a camera.
