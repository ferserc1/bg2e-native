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
in the frame loop: after the GPU is idle and before any traversal begins.

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

Queue a lambda to modify the scene on the next frame:

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
An entry queued without a token remains unconditional.

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

## Real Example: model_edit Environment Restore

The `model_edit` application allows the user to restore environment settings (skybox,
lights) from a file. This operation replaces a subtree of the scene graph, which
must happen when the GPU is idle and no traversal is active.

### StageScene Members

```cpp
class StageScene {
    // ...
    bool _restoringEnvironment = false;
    std::shared_ptr<bg2e::app::SafeUpdateToken> _restoreToken;
};
```

### restoreEnvironmentSettings Implementation

```cpp
void StageScene::restoreEnvironmentSettings(const std::filesystem::path& path)
{
    // Guard: prevent multiple concurrent restores
    if (_restoringEnvironment) return;
    _restoringEnvironment = true;

    // Load the new scene from file (on the main thread, before queuing)
    auto newScene = bg2e::db::loadScene(path);

    // Find the environment and lights nodes in the loaded scene
    auto findEnvironment =
        std::make_shared<FindNodeComponentVisitor<EnvironmentComponent>>();
    auto environmentNode = findEnvironment->find(newScene->rootNode());

    auto findLights = std::make_shared<FindNodeByProperties>();
    findLights->byName("Lights");
    auto lightNodes = findLights->find(newScene->rootNode());

    if (!environmentNode.empty() && lightNodes.size() == 1)
    {
        // Create a cancellation token
        _restoreToken = std::make_shared<bg2e::app::SafeUpdateToken>();

        // Queue the scene mutation
        bg2e::app::MainLoop::current()->safeUpdateScene(
            [this, environmentNode, newScene, lightNodes]()
            {
                auto envNode = environmentNode[0].lock();
                if (!envNode) {
                    _restoringEnvironment = false;
                    return;
                }

                // Swap the environment subtree
                _sceneRoot->removeChild(_environmentNode);
                _environment = std::dynamic_pointer_cast<EnvironmentComponent>(
                    envNode->environment()->shared_from_this());
                _environmentNode = std::static_pointer_cast<Node>(
                    newScene->rootNode()->shared_from_this());
                _lightsNode = lightNodes[0];
                _sceneRoot->addChild(_environmentNode);
                _sceneRoot->scene()->updateLights();

                _restoringEnvironment = false;
            },
            _restoreToken   // pass the token
        );
    }
    else
    {
        _restoringEnvironment = false;
        // Show error...
    }
}
```

### Why Each Protection Exists

1. **Guard flag** (`_restoringEnvironment`): Prevents the user from triggering
   multiple restores in the same frame (e.g., double-clicking "Restore Settings").
   Without this, two lambdas would be queued and the second would try to remove a
   node that the first already replaced, corrupting the scene tree.

2. **Explicit capture** (`[this, environmentNode, newScene, lightNodes]`): Makes
   it clear which objects the lambda depends on. The `[&]` capture was replaced
   because it implicitly captures `this` and all referenced members by reference,
   making lifetime analysis difficult.

3. **SafeUpdateToken** (`_restoreToken`): If `StageScene` is destroyed before the
   lambda executes (e.g., the user closes the application right after clicking
   "Restore"), the token is destroyed and the queued weak reference expires, so
   the lambda is discarded. Without this, the lambda would access a destroyed
   `this` pointer.

4. **`newScene` captured by value**: The loaded scene is captured by `shared_ptr`
   value in the lambda, ensuring the scene graph stays alive until the lambda
   executes and transfers ownership to `_sceneRoot`.

## Summary

| Function | Purpose |
|----------|---------|
| `safeUpdateScene(fn)` | Queue a scene mutation for the next frame |
| `safeUpdateScene(fn, token)` | Queue with cancellation support |
| `SafeUpdateToken` | RAII token — destroyed token cancels the lambda |
| `executeSafeUpdateScene()` | Internal: runs all valid lambdas at frame start |

Use `safeUpdateScene` whenever you need to add, remove, or replace nodes in the
scene graph outside of the initialization phase. Use a `SafeUpdateToken` when the
lambda captures `this` or references to objects that may be destroyed before the
next frame.

## Draw draw scene path

The separate [draw renderer lifecycle](api/draw/RendererDeferred.md) and [live settings contract](api/draw/Settings.md)
cover deferred scene ownership, public injection and queued safe changes.
Production render behavior described above remains supported.

## Draw draw histories and settings

Draw edits use DefaultRenderLoopDelegate::enqueueRendererUpdate with current
renderer generation at the synchronized pre-acquisition boundary. Use the callback's
Renderer::invalidateHistories before a possibly partial multi-object mutation,
and request scene work; do not capture an old renderer pointer. The built-in
signature detects many identities/value changes, but in-place uploads/custom shader
changes and same-camera teleports still require explicit notification. Settings
catalog/window edits use the same safe queued path. Renderer generation is not a
replacement for document/import/bake-job generations.

See [renderer lifecycle](api/draw/RendererDeferred.md), [settings](api/draw/Settings.md)
and [temporal ownership](api/draw/TemporalAccumulator.md). This addition does not
migrate production safeUpdateScene callers or create a draw bake scheduler.


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

Draw Composer checks request/document generations and weak node membership independently of renderer generation. Graph/component intent callbacks queue work at the stopped boundary; they do not make a captured raw node safe after document replacement. Imports parse CPU inputs on owned readers and realize graph/images/GPU resources on the main thread. MainLoop workers are joined before graphics cleanup; app import/HTTP readers have their own stop/join lifecycle. See [draw import lifecycle](apps/composer/draw_import_lifecycle.md).

Integrated scene baking freezes the batch source, finishes all targets, then prepares/adopts maps. Complete DB save publication and partial interactive material adoption have different guarantees; see [Composer](apps/draw_composer.md).


## Standalone draw source freezing

[Standalone contexts/batches](api/bake/StandaloneBakerContext.md) require private
retained scene/job owners with all producers stopped. Prepare UV2 and resources
before freezing, do not adopt one target's map mid-batch, and invalidate/reprepare
new source generations after edits. CPU cancellation/current callbacks do not
authorize competing GPU submissions. Full staged file publication is a separate
[writer transaction](api/bake/LightmapOutputWriter.md), not an interactive partial
material rollback or a replacement for exact recording completion.
