# UV preview, safe reload and editor bake integration

## UvMapPreviewRenderer

The preview is independent of the bake surface pass and `bg2e::ui`. It expands
selected CPU UV1/UV2 indexed triangles into a temporary non-indexed vertex list
with barycentrics `(1,0,0)`, `(0,1,0)`, `(0,0,1)` and a flat submesh ID. Malformed
index triangles are skipped; absent submesh metadata falls back to the whole
index buffer. This is a diagnostic drawing operation, not validity enforcement.

Two graphics pipelines draw a RGBA8 target: a triangle pipeline for tinted
coverage/wireframe and a line-list pipeline for the unit-square border.
The triangle fragment shader uses minimum interpolated barycentric coordinate
and `smoothstep` to reveal edges over dim hashed submesh colors. It does not
require polygon-mode wireframe. The threshold is `1.5 / resolution` in barycentric
space, so it is not a constant screen-pixel width for every triangle shape.
The vertex shader uses the same UV inversion/viewport convention as the bake.

The renderer performs `immediateSubmit()` and waits before releasing the upload
buffer, leaving its owned `render::Texture` in shader-read layout. Resolution
changes wait for idle and replace that texture. Consumers borrow the returned
pointer and must drop descriptors before replacement or renderer destruction.

Sources: [renderer](../../../lib/src/bg2e/render/UvMapPreviewRenderer.cpp),
[vertex shader](../../../shaders/src/glsl/uv_map_preview.vert.glsl),
[fragment shader](../../../shaders/src/glsl/uv_map_preview.frag.glsl).

## UvMapPreview

The UI wrapper owns a renderer and `TextureWidgets` descriptor path, and retains
only a shared CPU mesh. It defaults to UV2 and a 256-pixel display. `setMesh()`,
changing UV set/resolution, or `refresh()` marks it dirty. Passing the same mesh
pointer to `setMesh()` does not mark dirty; after in-place atlas generation use
`refresh()` explicitly.

On a dirty draw, it clears the previous descriptor, validates the selected UV
channel, renders the preview and attaches a non-owning texture reference to the
image widget. It displays coverage or the diagnostic. Resolution replacement
clears the descriptor first; cleanup releases the image widget before the renderer.
Public headers never expose ImGui types; drawing uses engine UI wrappers.
Without a mesh/renderer, draw reports no target rather than rendering.

Source: [widget implementation](../../../lib/src/bg2e/ui/UvMapPreview.cpp).

## Uv2SafeReload and Uv2RegenerationResult

A loaded Drawable cannot have its vertex/index resources replaced while frames
refer to them. `Uv2SafeReload::regenerate()` captures a weak node, expected root,
options and completion callback and queues CPU generation plus `Drawable::reload()`
through `MainLoop::safeUpdateScene`. The loop reaches a device-idle safe point
before executing the mutation. The helper checks that the node still exists,
retains the same root and has a standard loaded Drawable/mesh. It checks the
submesh count after reload and returns atlas metrics on success.

A returned non-null `SafeUpdateToken` controls queued-work lifetime. Releasing
its last strong reference cancels pending work; a completion already executing
is not cancelled. An already-expired node is reported immediately on the caller's
thread; normal queued completion runs at the main-thread safe point. Exceptions
from generation/reload are converted into a failed `Uv2RegenerationResult`.
CPU generation is transactional, but a later GPU reload failure does not restore
the old CPU mesh. The helper requires a running `MainLoop` and does not invalidate
bakers or mark documents dirty on its own.

```cpp
// Pattern used by the editor windows; keep token as an owner member.
uv2Token = bg2e::app::Uv2SafeReload::regenerate(targetNode, options,
    [this](const bg2e::app::Uv2RegenerationResult& result) {
        if (result.success) {
            preview.refresh();
            // Mark document dirty; bake jobs were cancelled before scheduling.
        }
    });
// On window cleanup or scene replacement:
uv2Token.reset();
```

Adapted from [helper](../../../lib/src/bg2e/app/Uv2SafeReload.cpp),
[model window](../../../apps/model_edit/src/ModelLightmapWindow.cpp),
[scene window](../../../apps/bg2e_composer/src/SceneLightmapWindow.cpp).

## ModelLightmapWindow and SceneLightmapWindow

These are application-owned `ui::Window` subclasses, not engine public classes.
`ModelLightmapWindow` targets the active model, owns its UV preview and one safe
reload token, and requests RTAO bakes. `SceneLightmapWindow` collects directly
attached Drawables, retains selection weakly, owns UV previews/reload tokens,
and requests selected-target RTAO or RTGI bakes. Both expose UV padding separately
from lightmap dilation. Composer also exposes GI ray bias and RGB8 export exposure.
Generation cancels active bakes before scheduling reload and refreshes previews
and document dirty state through completion.

The corresponding `AppDelegate` classes own bake jobs/context. Their render
callbacks first render the ordinary scene and then prepare/update the bake in
the same command stream. Composer shares one preparation across its jobs.
After the final recorded sample, delegates mark readback pending and finish it
from a later update, avoiding readback before frame submission. Completion writes
a temporary PNG, sets AO texture/UV2/unit scale on each material, refreshes GPU
material textures and marks the document dirty. Existing editor save workflows
persist the model; the CLI writer is not used for these temporary preview files.
Cancelling releases bake resources through deferred cleanup.

Sources: [model window header](../../../apps/model_edit/src/ModelLightmapWindow.hpp),
[model window](../../../apps/model_edit/src/ModelLightmapWindow.cpp),
[model delegate](../../../apps/model_edit/src/AppDelegate.cpp),
[scene window header](../../../apps/bg2e_composer/src/SceneLightmapWindow.hpp),
[scene window](../../../apps/bg2e_composer/src/SceneLightmapWindow.cpp),
[Composer delegate and bake jobs](../../../apps/bg2e_composer/src/AppDelegate.hpp),
[Composer execution](../../../apps/bg2e_composer/src/AppDelegate.cpp).
See [material limitation](output_and_materials.md) before treating AO assignment
as a dedicated RGB illumination material API.
