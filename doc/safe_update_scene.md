# Safe Update Scene API

## Motivation

In a real-time rendering application, the scene graph is continuously traversed by
visitors (UpdateVisitor, DrawVisitor, InputVisitor, RenderQueueVisitor, etc.) during
every frame. Modifying the scene tree — adding or removing nodes, swapping
components, or replacing subtrees — while a traversal is in progress leads to
undefined behavior: dangling pointers, corrupted iteration state, or use-after-free
crashes.

Some operations also require the GPU to be idle before scene resources can be safely
released. For example, removing a node that owns Vulkan textures or buffers must wait
until the device has finished using those resources.

`safeUpdateScene` solves both problems by deferring scene mutations to a safe point
in the frame loop: after draining the active execution and before frame rendering.
This does not exclude independent CPU readers or background GPU producers; the
application must stop those before changing their sources.

## How It Works

### Frame Loop

The following is a schematic of the MainLoop ordering; focus throttling, timers,
main-thread work and renderer preparation remain part of the actual loop:

```
1. Process SDL events (input, window events)
2. executeSafeUpdateScene()    ← deferred scene mutations run here
3. Drain main-thread work and due timers
4. Active execution prepares/acquires/renders/presents when a frame is due
```

`executeSafeUpdateScene()` does:

```cpp
void MainLoop::executeSafeUpdateScene()
{
    std::vector<SafeUpdateSceneEntry> local;
    {
        std::lock_guard lock(_safeUpdateSceneMutex);
        if (_safeUpdateScene.empty())
        {
            return;
        }
        std::swap(local, _safeUpdateScene);
    }
    // The execution exists only during an active run; outside a run there is
    // no GPU work in flight to wait for.
    if (_execution)
    {
        base::CpuProfileScope profile("app.waitIdle.safeSceneUpdate");
        base::CpuProfileScope::count("app.safeSceneUpdatesWaiting", local.size());
        _execution->waitIdle();
    }
    bool updated = false;
    for (auto& entry : local)
    {
        if (!entry.hasToken)
        {
            entry.function();
            updated = true;
            continue;
        }

        auto token = entry.token.lock();
        if (token && token->alive->load())
        {
            entry.function();
            updated = true;
        }
    }
    if (updated && _execution)
    {
        requestSceneFrame();
    }
}
```

Key properties:

- A mutex-protected snapshot of queued entries is consumed in order; callbacks
  queued while that snapshot executes wait for a later drain. Expired tokens skip
  their callbacks. Keep callbacks nonthrowing if the rest of the snapshot must run.
- A nonempty snapshot drains the active execution once before callbacks.
  Outside an active run there is no execution to drain. Stop background producers;
  waitIdle reopens submission admission and is not a permanent device freeze.
- Lambdas execute on the **main thread**, synchronously, before rendering.
- Entries are removed from the shared queue before execution. A throwing callback
  propagates and later local entries are not automatically retried. Successful
  callbacks request a scene refresh when an execution is active.

### Basic Usage

Queue a lambda to modify the scene at the next main-loop safe drain:

```cpp
bg2e::app::MainLoop::current()->safeUpdateScene([&]() {
    sceneRoot->removeChild(oldNode);
    sceneRoot->addChild(newNode);
});
```

This small fragment assumes sceneRoot/oldNode/newNode remain alive until the
callback runs. In an editor, use retained/weak owners and an appropriate validity
token instead of capturing short-lived references. The mutation runs at the next
safe drain, not during traversal.

## The SafeUpdateToken

### Problem: Stale Lambdas

A lambda queued via `safeUpdateScene` captures `this` (or references to objects)
that may be destroyed before the lambda executes. If the owner object is destroyed
between queuing and execution, the lambda accesses dangling pointers.

Example of the problem:

```cpp
class MyScene {
    std::shared_ptr<Node> _root;

    void replaceSubtree(std::shared_ptr<Node> newNode) {
        // Lambda captures 'this' implicitly via [&]
        MainLoop::current()->safeUpdateScene([&, newNode]() {
            _root->removeChild(_oldNode);  // 'this' may be destroyed!
            _root->addChild(newNode);
        });
    }
};

// If MyScene is destroyed before the next frame, the lambda crashes.
```

### Solution: Cancellation Token

`SafeUpdateToken` is a lightweight RAII token that signals whether a queued lambda
is still valid to execute.

```cpp
struct SafeUpdateToken {
    std::shared_ptr<std::atomic<bool>> alive =
        std::make_shared<std::atomic<bool>>(true);
    ~SafeUpdateToken() { *alive = false; }
};
```

The queue stores a weak reference to the token rather than extending its
lifetime. When the caller releases the last token reference (e.g., when the
owner object is destroyed), the token destructor sets `alive` to `false` and
the weak reference expires. `executeSafeUpdateScene()` skips that queued lambda.
An entry queued without a token remains unconditional. Retain the token in the
owner until execution is wanted: passing a temporary token cancels the entry as
soon as that temporary is released. Do not capture the token strongly in the
callback, because that would keep it alive and defeat owner-based cancellation.

A token is a cancellation check, not a lock or an ownership guarantee for `this`.
It cannot prevent concurrent owner destruction after the check. Keep owner
lifecycle on the main thread, or lock a weak owner in the callback. Likewise,
captured `shared_ptr` values keep objects alive but do not establish that they
still belong to the current document.

### API

```cpp
// Queue without token (no cancellation — use for guaranteed-lifetime contexts)
MainLoop::current()->safeUpdateScene(fn);

// Queue with token (lambda is discarded if token is destroyed before execution)
auto token = std::make_shared<SafeUpdateToken>();
MainLoop::current()->safeUpdateScene(fn, token);
```

### When to Use the Token

| Scenario | Use token? |
|----------|-----------|
| Lambda captures `this` and the owner object might be destroyed | **Yes** |
| Lambda only captures values (shared_ptr, copies) | No |
| Lambda modifies a singleton or global state | No |
| Lambda is queued from a delegate that has cleanup/destroy lifecycle | **Yes** |

## Current Example: Composer Node Edits

`apps/bg2e_composer` uses `draw::DefaultRenderLoopDelegate` update queues rather
than `MainLoop::safeUpdateScene` for editor mutations. The implementation in
[EditorActions.cpp](../apps/bg2e_composer/src/EditorActions.cpp) centralizes node
edits in `AppDelegate::enqueueNodeEdit`:

1. Capture the current document generation, a weak delegate and a weak node.
2. Choose `enqueueRendererUpdate` for arbitrary changes, or
   `enqueueCpuValueUpdate` for CPU values consumed by future frame snapshots.
3. At execution, lock the owner and node, check the document generation and
   membership in the current stage (with an explicit fill-light exception).
4. Invalidate lightmap sources before the operation. Rebuild tools and refresh
   selection when requested.
5. Call `notifyEdited(renderer)` even after a caught `std::exception`, since an
   operation may have changed part of its target. This marks the stage modified,
   invalidates temporal histories, refreshes the camera and requests a scene frame.
   It keeps the current scene attached; document replacement has a separate lifecycle.

`queueNodeEdit` selects the conservative path for geometry modifiers, hierarchy
changes and resource edits. `queueNodeValueEdit` selects the CPU path for supported
value edits, including node names and submesh metadata. Camera intrinsics and
highlight intensity also use `enqueueCpuValueUpdate` directly.

The material editor opts into CPU value and retained resource edits, with a
weak owner/document-generation guard and an `onChanged` notification to
`notifyEdited`. These opt-ins rely on the material editor's specific snapshot
and resource-retention contracts; they do not make arbitrary callbacks safe.

Environment restore is also queued conservatively. `environmentPreset` validates
the loaded preset, synchronizes a candidate through the active deferred renderer's
environment generator, then adopts it and refreshes tools/selection. A failure
after adoption still marks the stage modified and invalidates histories. This
is the current Composer flow, rather than the older `model_edit` restore example.

## Summary

| Function | Purpose |
|----------|---------|
| `safeUpdateScene(fn)` | Queue a scene mutation for the next main-loop safe drain |
| `safeUpdateScene(fn, token)` | Queue with cancellation support |
| `SafeUpdateToken` | RAII token — destroyed token cancels the lambda |
| `executeSafeUpdateScene()` | Internal: runs all valid lambdas at frame start |

Use `safeUpdateScene` whenever you need to add, remove, or replace nodes in the
scene graph outside of the initialization phase. Use a `SafeUpdateToken` when the
lambda captures `this` or references to objects that may be destroyed before the
next frame.

## Draw Scene and Renderer Lifecycle

The separate [draw renderer lifecycle](api/draw/RendererDeferred.md) and [live settings contract](api/draw/Settings.md)
cover deferred scene ownership, public injection and queued safe changes.
Production render behavior described above remains supported.

## Draw Update Queues, Histories and Settings

Draw queues run on the main thread in `DefaultRenderLoopDelegate::prepareFrame`,
before acquisition and frame UI/update callbacks. They are separate from the
mutex-protected MainLoop queue: enqueue draw work on the main thread. Each prepare
hook takes a snapshot; work enqueued by its callbacks waits for a later hook.

| Queue | Synchronization contract |
|-------|--------------------------|
| `enqueueRendererUpdate` | Conservative: a batch containing any arbitrary callback opens a `gpu::Device::ResourceUpdateScope` before execution, including stale callbacks. |
| `enqueueCpuValueUpdate` | May skip the drain for CPU values copied into future frame snapshots. No permission to change native resources or submit GPU work without separate synchronization. |
| `enqueueRetainedResourceUpdate` | May skip the drain when publishing resource versions. Retain replaced native owners until their submissions finish; never overwrite resources referenced by submitted or unsubmitted commands. Uploads synchronize their own dependencies. |
| `enqueueFinalColorUpdate` | Bounded typed route for supported standard final-pass exposure, contrast, brightness and gamma. Unsupported configurations return `false` without queueing; use a conservative fallback. |

Mixed batches containing a conservative callback still synchronize and preserve
queue order. `setValueUpdateDrainElisionEnabled(false)` forces typed batches to
drain as a diagnostic baseline. Background producers must remain stopped; GPU
completion does not provide CPU-thread exclusion.

Generic callbacks validate renderer generation and target binding through
`withRenderer`; stale work invokes the optional stale callback. A successful
callback requests a scene frame. Do not capture an old renderer pointer: use the
renderer passed to the callback. Renderer generation does not replace weak owner,
document, import or bake-job validity checks.

Scene/renderer replacements queued by `setScene`/`setRenderer` commit at the draw
prepare boundary under their own resource-update scope. They are not committed
synchronously by a MainLoop safe-update callback. Rebinding changes renderer target
validity; updates from an old generation can therefore become stale.

Arbitrary edits must explicitly invalidate histories when needed, including after
possibly partial changes. The built-in signature detects many identity/value
changes, but in-place uploads/custom shader changes and same-camera teleports
still need notification. Composer's node-edit path performs this through
`notifyEdited`; this is not an automatic property of every queued operation.

Settings use the same queue infrastructure, with a typed route for the four
supported final color properties and conservative fallback for other changes.
See [update synchronization](api/draw/UpdateSynchronization.md),
[renderer lifecycle](api/draw/RendererDeferred.md), [settings](api/draw/Settings.md)
and [temporal ownership](api/draw/TemporalAccumulator.md). These paths do not
implicitly migrate `safeUpdateScene` callers or create a draw bake scheduler.

## Integrated draw bake edits

The [draw lightmap tutorial](tutorials/draw/12_lightmaps.md) freezes scene,
material, light and environment sources while samples are prepared/submitted.
Before a queued UV2/scene/material replacement, retire submitted work or discard
and abandon an unsubmitted frame. Then invalidate sources/reset target state or
release/recreate the context. Reacquire replaced drawable/tracing/selection/preview
owners after adoption. A settings generation/token does not establish GPU
completion. The example captures all targets' completed GPU and typed CPU outputs
before material adoption; applying one map mid-batch would invalidate its baseline.

## Composer document and worker validity

Draw Composer checks request/document generations and weak node membership independently of renderer generation. Graph/component intent callbacks queue work at the pre-acquisition boundary with the synchronization appropriate to the edit; they do not make a captured raw node safe after document replacement. Imports parse CPU inputs on owned readers and realize graph/images/GPU resources on the main thread. MainLoop workers are joined before graphics cleanup; app import/HTTP readers have their own stop/join lifecycle. See [draw import lifecycle](apps/composer/draw_import_lifecycle.md).

Integrated scene baking freezes the batch source, finishes all targets, then prepares/adopts maps. Complete DB save publication and partial interactive material adoption have different guarantees; see [Composer](apps/draw_composer.md).


## Standalone draw source freezing

[Standalone contexts/batches](api/bake/StandaloneBakerContext.md) require private
retained scene/job owners with all producers stopped. Prepare UV2 and resources
before freezing, do not adopt one target's map mid-batch, and invalidate/reprepare
new source generations after edits. CPU cancellation/current callbacks do not
authorize competing GPU submissions. Full staged file publication is a separate
[writer transaction](api/bake/LightmapOutputWriter.md), not an interactive partial
material rollback or a replacement for exact recording completion.
