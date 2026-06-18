# Plan Status

## Step 1 completed: Node::deserialize() progress parameter
Date: 2026-06-18
Changes:
- lib/include/bg2e/scene/Node.hpp: Added `#include <functional>`, type alias `SceneProgressCallback`, structs `SceneLoadProgress` and `SceneSaveProgress`; updated `deserialize()` signature with `SceneLoadProgress* progress = nullptr`
- lib/src/bg2e/scene/Node.cpp: Updated `deserialize()` implementation to read name before firing callback, invoke progress callback with node name/processed/total before heavy work, increment counter after callback, forward `progress` pointer to recursive child deserialization

## Step 2 completed: Node::serialize() progress parameter
Date: 2026-06-18
Changes:
- lib/include/bg2e/scene/Node.hpp: Updated `serialize()` signature with `SceneSaveProgress* progress = nullptr`
- lib/src/bg2e/scene/Node.cpp: Updated `serialize()` implementation to fire progress callback before serializing each node, increment counter after callback, and forward `progress` pointer to recursive child serialization

## Step 3 completed: Scene::deserialize() progress parameter
Date: 2026-06-18
Changes:
- lib/include/bg2e/scene/Scene.hpp: Updated `deserialize()` signature with `SceneLoadProgress* progress = nullptr`
- lib/src/bg2e/scene/Scene.cpp: Added `progress` parameter to implementation, forwarded it into each `node->deserialize()` call

## Step 4 completed: Internal counting helpers in db/scene.cpp
Date: 2026-06-18
Changes:
- lib/src/bg2e/db/scene.cpp: Added static `countJsonNodes()` to count JSON node tree for loadScene pre-counting; added static `countSceneNodes()` to count live scene tree for saveScene pre-counting

## Step 5 completed: Public API onProgress parameter
Date: 2026-06-18
Changes:
- lib/include/bg2e/db/scene.hpp: Added `SceneProgressCallback onProgress = nullptr` as last parameter to all four public functions
- lib/src/bg2e/db/scene.cpp: Updated all four function signatures; primary `loadScene` builds `SceneLoadProgress` struct and pre-counts JSON nodes before calling `Scene::deserialize`; primary `saveScene` builds `SceneSaveProgress` struct and pre-counts scene nodes before calling `serialize`; secondary overloads forward `onProgress`; fixed pre-existing `basePath += fileName` bug to `basePath / fileName` in secondary `saveScene`
