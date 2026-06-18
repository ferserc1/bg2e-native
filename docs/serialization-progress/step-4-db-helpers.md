# Step 4 — Internal counting helpers in `db/scene.cpp`

## File

- `lib/src/bg2e/db/scene.cpp`

These are `static` functions — not exported, not declared in any header.

## `countJsonNodes` — for `loadScene`

Counts the total number of nodes in the JSON tree before deserialization begins. The JSON format represents a node as an object with an optional `"children"` list of nested node objects.

```cpp
static int countJsonNodes(std::shared_ptr<bg2e::json::JsonNode> jsonNode)
{
    if (!jsonNode || !jsonNode->isObject()) { return 0; }
    auto& obj = jsonNode->objectValue();
    int count = 1;  // this node
    if (obj.count("children") && obj["children"]->isList()) {
        for (auto& child : obj["children"]->listValue()) {
            count += countJsonNodes(child);
        }
    }
    return count;
}
```

Called once per top-level node in the `"scene"` array, before `Scene::deserialize()`:

```cpp
if (onProgress) {
    if (sceneObj.count("scene") && sceneObj["scene"]->isList()) {
        for (auto& nodeData : sceneObj["scene"]->listValue()) {
            progress.total += countJsonNodes(nodeData);
        }
    }
}
```

## `countSceneNodes` — for `saveScene`

Counts the total number of nodes in the live scene tree before serialization begins.

```cpp
static int countSceneNodes(bg2e::scene::Node* node)
{
    if (!node) { return 0; }
    int count = 1;  // this node
    for (auto& child : node->children()) {
        count += countSceneNodes(child.get());
    }
    return count;
}
```

Called once before `sceneRoot->serialize()`:

```cpp
if (onProgress) {
    progress.total = countSceneNodes(sceneRoot);
}
```

## Why count upfront

The recursive `deserialize`/`serialize` calls consume the tree in a single pass. To report meaningful progress (`loaded / total`) the total must be known before the pass starts. Both functions are O(n) in the number of nodes, which is negligible compared to the GPU resource allocation and I/O that follows.
