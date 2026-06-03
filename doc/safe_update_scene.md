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

The main loop in `MainLoop::run()` executes the following sequence each frame:

```
1. Process SDL events (input, window events)
2. executeSafeUpdateScene()    ← deferred scene mutations run here
3. acquireAndPresent()         ← rendering (traversals, draw calls)
```

`executeSafeUpdateScene()` does:

```cpp
void MainLoop::executeSafeUpdateScene()
{
    _engine.device().waitIdle();           // wait for GPU
    for (auto& [fn, token] : _safeUpdateScene)
    {
        if (!token || token->alive->load())
        {
            fn();                          // execute deferred lambda
        }
    }
    _safeUpdateScene.clear();              // flush the queue
}
```

Key properties:

- All queued lambdas execute **once**, in order, at the start of the next frame.
- The GPU is guaranteed idle before any lambda runs.
- Lambdas execute on the **main thread**, synchronously, before rendering.
- The queue is cleared after execution — lambdas run exactly once.

### Basic Usage

Queue a lambda to modify the scene on the next frame:

```cpp
bg2e::app::MainLoop::current()->safeUpdateScene([&]() {
    sceneRoot->removeChild(oldNode);
    sceneRoot->addChild(newNode);
});
```

The lambda captures whatever it needs. The scene modification happens safely at the
start of the next frame.

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

When the token is destroyed (e.g., when the owner object is destroyed), `alive`
becomes `false`. `executeSafeUpdateScene()` checks this flag before running each
lambda and silently discards invalidated lambdas.

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
   "Restore"), the token is destroyed → `alive` becomes `false` → the lambda is
   discarded. Without this, the lambda would access a destroyed `this` pointer.

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
