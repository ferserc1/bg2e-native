# Step 2 — `Node::serialize()` progress parameter

## Files

- `lib/include/bg2e/scene/Node.hpp`
- `lib/src/bg2e/scene/Node.cpp`

## Header change (`Node.hpp`)

Update the existing declaration inside `class Node`:

```cpp
// before
std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&);

// after
std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&,
                                          SceneSaveProgress* progress = nullptr);
```

`SceneSaveProgress` is defined in Step 1 (same header).

## Implementation change (`Node.cpp`)

Current implementation starts at line 283.

```cpp
std::shared_ptr<json::JsonNode> Node::serialize(const std::filesystem::path& basePath,
                                                SceneSaveProgress* progress)
{
    // Fire callback before serializing this node
    if (progress && progress->callback) {
        progress->callback(_name, progress->saved, progress->total);
    }
    if (progress) { ++progress->saved; }

    using namespace bg2e::json;
    auto result = JSON(JsonObject{});
    auto& obj = result->objectValue();

    obj["type"]    = JSON("Node");
    obj["name"]    = JSON(name());
    obj["enabled"] = JSON(enabled());
    obj["steady"]  = JSON(steady());

    // Serialize components (unchanged)
    auto components = JSON(JsonList());
    for (auto& comp : _components) {
        if (comp.second->ignoreSerialization()) { continue; }
        auto compData = comp.second->serialize(basePath);
        components->listValue().push_back(compData);
    }
    obj["components"] = components;

    // Serialize children — forward the same progress pointer
    auto children = JSON(JsonList());
    for (auto& child : _children) {
        auto childData = child->serialize(basePath, progress);
        children->listValue().push_back(childData);
    }
    obj["children"] = children;

    return result;
}
```

## Why the callback fires before serialization

Serializing a node with a `DrawableComponent` involves reading mesh and material data back into JSON. Firing before this work follows the same pattern as `deserialize`: the message "saving 'X'…" is visible while the work happens.
