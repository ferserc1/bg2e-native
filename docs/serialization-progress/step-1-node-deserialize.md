# Step 1 — `Node::deserialize()` progress parameter

## Files

- `lib/include/bg2e/scene/Node.hpp`
- `lib/src/bg2e/scene/Node.cpp`

## Header change (`Node.hpp`)

Add the type alias, the two context structs, and the updated signature. All additions go at the top of the `bg2e::scene` namespace, before the `Node` class declaration.

```cpp
using SceneProgressCallback = std::function<void(const std::string& nodeName, int processed, int total)>;

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

Update the existing declaration inside `class Node`:

```cpp
// before
void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&, render::Engine& engine);

// after
void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&,
                 render::Engine& engine, SceneLoadProgress* progress = nullptr);
```

## Implementation change (`Node.cpp`)

Current implementation starts at line 231. The name must be read **before** calling the callback so the message is meaningful.

```cpp
void Node::deserialize(std::shared_ptr<json::JsonNode> jsonData,
                       const std::filesystem::path& basePath,
                       render::Engine& engine,
                       SceneLoadProgress* progress)
{
    if (!jsonData || !jsonData->isObject()) { return; }
    auto& obj = jsonData->objectValue();

    // Read name first so the callback has it
    if (obj.count("name")) {
        _name = obj["name"]->stringValue();
    }

    // Fire callback before the heavy component/child work
    if (progress && progress->callback) {
        progress->callback(_name, progress->loaded, progress->total);
    }
    if (progress) { ++progress->loaded; }

    if (obj.count("enabled")) { _disabled = !obj["enabled"]->boolValue(); }
    if (obj.count("steady"))  { _steady   =  obj["steady"]->boolValue();  }

    // Deserialize components (unchanged)
    if (obj.count("components") && obj["components"]->isList()) {
        for (auto& compData : obj["components"]->listValue()) {
            auto* comp = ComponentFactoryRegistry::get().create(compData, basePath, engine);
            if (comp) { addComponent(comp); }
        }
    }

    // Deserialize children — forward the same progress pointer
    if (obj.count("children") && obj["children"]->isList()) {
        for (auto& childData : obj["children"]->listValue()) {
            auto childNode = std::make_shared<Node>();
            childNode->deserialize(childData, basePath, engine, progress);
            addChild(childNode);
        }
    }
}
```

## Why the callback fires before component loading

Components (especially `DrawableComponent`) trigger GPU resource allocation and are the heaviest part of node loading. Firing before that work means the UI message "loading 'X'…" appears while the work is happening, not after it has already finished.
