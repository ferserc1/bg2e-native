# Uv2SafeReload

**Header:** `<bg2e/app/Uv2SafeReload.hpp>`
**Namespace:** `bg2e::app`

`Uv2SafeReload` schedules CPU UV2 generation and GPU resource reload for an
already loaded standard scene Drawable. It queues the work through
`MainLoop::safeUpdateScene`, so the operation runs on the main thread at the
safe scene-update point after the engine waits for GPU work to finish.

```cpp
std::shared_ptr<app::SafeUpdateToken> uv2Token;

geo::Uv2AtlasOptions options;
options.resolution = 1024;
options.paddingPixels = 8;

uv2Token = app::Uv2SafeReload::regenerate(
    targetNode,
    options,
    [this](const app::Uv2RegenerationResult& result) {
        if (result.success)
        {
            // The Drawable has been reloaded; invalidate bake history and
            // refresh any UV preview here.
        }
        else
        {
            // Display result.message.
        }
    });
```

`targetNode` is accepted as a `std::weak_ptr<scene::Node>`; a `shared_ptr` can
be passed and converted. It must have a standard `scene::Drawable` component
with a loaded mesh, attached to the same scene root when the queued operation
runs. The helper invokes `GenerateUv2AtlasModifier::apply()` and then
`Drawable::reload()`. The callback runs on the main thread after success or a
reported failure, unless the token is destroyed before queued work begins. If
the weak node is already expired when `regenerate()` is called, its failure
callback runs immediately on the calling thread; if the node expires while
queued, the failure callback runs at the safe update point.

## Cancellation and result

Retain the returned non-null `SafeUpdateToken` while the operation is wanted.
The main-loop queue keeps only a weak reference, so destroying/resetting the
last token reference cancels queued work, for example when a window closes or a
scene is replaced. A window callback that captures `this` must be paired with
this cancellation lifetime rule. A callback already in progress cannot be
cancelled. After success, invalidate active bake
accumulation and call `ui::UvMapPreview::refresh()` (or set a replacement
mesh) from the completion callback.

`Uv2RegenerationResult` contains `success`, `message`, `atlasWidth`,
`atlasHeight`, `chartCount`, and `utilization`. On success these atlas metrics
come from `geo::Uv2AtlasResult`; on failure `message` describes the reported
problem.

`GenerateUv2AtlasModifier::apply()` itself is transactional: if atlas
generation fails, the CPU mesh remains unchanged. The safe helper then reloads
GPU resources after a successful CPU update. If a GPU reload fails after the
CPU modifier has committed, the helper reports the failure but does not restore
the previous CPU mesh; callers should treat this exceptional reload failure as
a failed editor operation requiring recovery.

This API requires an active `MainLoop`. A headless caller should run the CPU
modifier before loading the target into GPU resources instead.

## See also

- [`MainLoop::safeUpdateScene`](MainLoop.md#safe-scene-updates)
- [`geo::GenerateUv2AtlasModifier`](../geo/GenerateUv2AtlasModifier.md)
- [`ui::UvMapPreview`](../ui/UvMapPreview.md)

Sources: [header](../../../lib/include/bg2e/app/Uv2SafeReload.hpp),
[implementation](../../../lib/src/bg2e/app/Uv2SafeReload.cpp).

## Signature

```cpp
using CompletionCallback = std::function<void(const Uv2RegenerationResult&)>;
static std::shared_ptr<SafeUpdateToken> regenerate(
    std::weak_ptr<scene::Node> targetNode,
    const geo::Uv2AtlasOptions& options = {},
    CompletionCallback onComplete = nullptr);
```

Result defaults are false success, empty message, zero width/height/chart count
and zero utilization. The callback is optional. Recreate bake resources after
UV topology changes; resetting accumulation alone does not repair stale scene
bindings. See [editor architecture](../../architecture/lightmaps/preview_and_editors.md).


## Draw-specific queued replacement

The existing regenerate() description above is the production path. The public
draw overload is:

```cpp
static std::shared_ptr<SafeUpdateToken> regenerateDraw(
    std::weak_ptr<draw::DefaultRenderLoopDelegate> delegate,
    std::weak_ptr<scene::Node> targetNode,
    const geo::Uv2AtlasOptions& options = {},
    CompletionCallback onComplete = nullptr,
    std::function<void()> beforeChange = {});
```

It queues independent drawable replacement with weak delegate/node, renderer
generation, scene membership and geometry identity guards. Retain the cancellation
token. beforeChange runs at the drained boundary to invalidate application-owned
bake/source state even if a later generation/adoption operation fails. Completion
must refetch drawable/mesh owners, selection and UV/result previews. A token alone
does not retire an open bake frame. Do not call a production preview widget with
draw resources. For direct stopped-boundary use see
[generateUv2AndReload](../bake/UvSurfacePass.md) and the
[draw lightmap tutorial](../../tutorials/draw/12_lightmaps.md).

Draw implementation: [DrawUv2SafeReload.cpp](../../../lib/src/bg2e/app/DrawUv2SafeReload.cpp).
