# UV surfaces and draw UV2 replacement

Headers: `<bg2e/bake/UvSurfaceMesh.hpp>`, `<bg2e/bake/UvSurfacePass.hpp>`,
`<bg2e/bake/Uv2Reload.hpp>`; included by bake/all.hpp. Production render is separate.

buildUvSurfaceMesh(mesh) validates texCoord1 and packs 64-byte deindexed vertices
with submesh and chart IDs. Shared source vertex-index edges connect within each
submesh; duplicate seam vertices and material/transform boundaries remain isolated. Tiny/zero UV faces
are ignored consistently with geo validation. These are generation-local,
conservative charts, not persistent document identities.

Initialize UvSurfacePass with Device/backend/slot count and optionally configured
shader root/prefix/interface bake.uvSurface.v1. At a stopped boundary prepare a
loaded draw Drawable, obtaining a retained immutable uploaded-geometry snapshot.
Call beginFrame(slot,serial) only after that slot completes, then record one or
more UvSurfaceInput targets. Input borrows CommandBuffer and nodeWorld, carries
source/target generation tags and extent, and starts outside scopes. It does not
submit. Generation chronology belongs to the surrounding job; changed uploaded
geometry is rejected and requires prepare again.

Each result owns fresh position RGBA32F, signed normal RGBA16F, submesh R32_UINT,
chart R32_UINT and R8 coverage images. Four depthless MRTs use 32 bytes/sample;
coverage is derived by compute from chart. Missing formats/limits/kernels fail
explicitly. UV v=0 is the top row, submesh is zero-based with UINT_MAX background,
chart is one-based with zero background. Shapes are geometric receivers, not alpha
cutout coverage. All submeshes participate, independently of viewport visibility.
World model includes node, Drawable and submesh transforms; inverse transpose
handles nonuniform/mirrored normal transforms. No scene camera or RT is needed.

result.surfaceInputs() borrows compatible public draw WorldPositionCoverage inputs.
Keep result owners alive through AO/GI/filter use. Submesh/chart maps remain separate
for composition/atlas isolation; current draw denoise does not consume chart
IDs. Invocation resources/outputs survive completed-slot reuse while retained;
Device must outlive them. Max invocations defaults to 64 per slot/serial. Cleanup
and preparation drain at stopped boundaries; recording does not waitIdle.

readDiagnostic(result) uses completed raw color readback and explicit integer,
float32 and half decoding. It does not infer GPU completion from CPU ownership.
See [native ABI/lifecycle](../../../docs/render_to_draw/plan/milestone_04_lightmaps/uv-surfaces.md)
and [manual validation](../../../tests/bake/uv_surface/README.md).

For a rare stopped edit, generateUv2AndReload(source,options) calls the existing
geo atlas generator/validator on staged CPU data and returns a deep independent
loaded Drawable. It preserves submesh/material association and clones authored GPU
resources, rebuilding raster/available BLAS identity. Publish only on success;
source/other attachments remain unchanged. Re-extract queues/tracing and invalidate
bake/source/target histories and selection/preview caches explicitly.

The application wrapper app::Uv2SafeReload::regenerateDraw(weakDelegate,weakNode,
options,onComplete,beforeChange) queues the operation at the draw synchronized
boundary. Retain its SafeUpdateToken; weak/generation/scene/geometry guards reject
stale edits. beforeChange invalidates active application-owned bake state before
possible mutation; completion must refetch changed drawable resources. Renderer
histories and scene redraw are coordinated. Production regenerate() is untouched.

The surface primitive itself does not schedule or apply maps. Use
[IntegratedBakerContext](IntegratedBakerContext.md), [finished output](LightmapOutput.md)
and the [UV preview](UvMapPreviewRenderer.md) for those delivered services.
