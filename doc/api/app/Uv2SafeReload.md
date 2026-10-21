# Uv2SafeReload

**Header:** `<bg2e/app/Uv2SafeReload.hpp>`

The public operation is regenerateDraw(); the old production regenerate() is removed.

## Draw-specific queued replacement

Queue an independent draw replacement:

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
does not retire an open bake frame. Use bake UV diagnostic/preview APIs for the replacement. For direct stopped-boundary use see
[generateUv2AndReload](../bake/UvSurfacePass.md) and the
[draw lightmap tutorial](../../tutorials/draw/12_lightmaps.md).

Draw implementation: [DrawUv2SafeReload.cpp](../../../lib/src/bg2e/app/DrawUv2SafeReload.cpp).
