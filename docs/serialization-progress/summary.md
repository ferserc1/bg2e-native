# Scene Serialization Progress Callbacks — Implementation Plan

## Goal

Add an optional progress callback to `loadScene` and `saveScene` in `bg2e::db`. When provided, the callback fires once per node (before the node's work begins), reporting the node name, the number already processed, and the total count. Callers that omit the callback see no behavioural change.

## Callback signature

```cpp
using SceneProgressCallback = std::function<void(const std::string& nodeName, int processed, int total)>;
```

Defined in `lib/include/bg2e/scene/Node.hpp` so it is visible to both the `scene` and `db` layers.

## Progress context structs

Two small structs carry mutable counter state through the recursive calls without polluting the public API with extra parameters:

```cpp
struct SceneLoadProgress {
    SceneProgressCallback callback;
    int loaded = 0;
    int total  = 0;
};

struct SceneSaveProgress {
    SceneProgressCallback callback;
    int saved = 0;
    int total = 0;
};
```

## Steps

| Step | File(s) | Description |
|------|---------|-------------|
| [1](step-1-node-deserialize.md) | `Node.hpp` / `Node.cpp` | Add `SceneLoadProgress*` to `Node::deserialize()` |
| [2](step-2-node-serialize.md) | `Node.hpp` / `Node.cpp` | Add `SceneSaveProgress*` to `Node::serialize()` |
| [3](step-3-scene-deserialize.md) | `Scene.hpp` / `Scene.cpp` | Forward `SceneLoadProgress*` through `Scene::deserialize()` |
| [4](step-4-db-helpers.md) | `db/scene.cpp` (internal) | Static helpers to pre-count nodes before recursive passes |
| [5](step-5-db-api.md) | `db/scene.hpp` / `db/scene.cpp` | Expose `SceneProgressCallback onProgress = nullptr` on all four public functions |

## Files to modify (in order)

| File | Change |
|------|--------|
| `lib/include/bg2e/scene/Node.hpp` | Add type aliases, structs; update signatures |
| `lib/src/bg2e/scene/Node.cpp` | Update `deserialize` and `serialize` implementations |
| `lib/include/bg2e/scene/Scene.hpp` | Update `deserialize` signature |
| `lib/src/bg2e/scene/Scene.cpp` | Forward `progress` pointer in `deserialize` |
| `lib/include/bg2e/db/scene.hpp` | Add `onProgress` param to all four functions |
| `lib/src/bg2e/db/scene.cpp` | Add count helpers; build progress structs; wire up |

## Additional requirements / caveats

- **Empty node names**: a node's name may be `""` if the JSON has no `"name"` key. The callback must tolerate this.
- **`total = 0` guard**: when `onProgress` is null the progress struct is never populated and a `nullptr` is passed; every internal check is `if (progress)`, so no null dereference is possible.
- **Pre-existing bug in `saveScene` two-argument overload**: it concatenates `basePath += fileName` with `+=` instead of `/`. Out of scope for this feature but worth a follow-up fix.
- **No thread-safety needed**: deserialization is single-threaded; `SceneLoadProgress` / `SceneSaveProgress` are stack-local and pointer-passed.
- **Component serialize/deserialize is out of scope**: components use inheritance and their `serialize`/`deserialize` API is not modified. Progress tracks nodes only.
