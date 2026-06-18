# Step 3 — `Scene::deserialize()` progress parameter

## Files

- `lib/include/bg2e/scene/Scene.hpp`
- `lib/src/bg2e/scene/Scene.cpp`

## Header change (`Scene.hpp`)

```cpp
// before
static std::shared_ptr<Scene> deserialize(
    std::shared_ptr<json::JsonNode>,
    const std::filesystem::path&,
    render::Engine& engine);

// after
static std::shared_ptr<Scene> deserialize(
    std::shared_ptr<json::JsonNode>,
    const std::filesystem::path&,
    render::Engine& engine,
    scene::SceneLoadProgress* progress = nullptr);
```

`SceneLoadProgress` is already available because `Scene.hpp` includes `Node.hpp`.

## Implementation change (`Scene.cpp`)

The only change is forwarding `progress` into each `node->deserialize()` call (line 57):

```cpp
std::shared_ptr<Scene> Scene::deserialize(std::shared_ptr<json::JsonNode> jsonData,
                                          const std::filesystem::path& basePath,
                                          render::Engine& engine,
                                          SceneLoadProgress* progress)
{
    // ... version logging unchanged ...

    auto sceneRoot = std::make_shared<Node>("scene root");

    if (obj.count("scene") && obj["scene"]->isList()) {
        for (auto& nodeData : obj["scene"]->listValue()) {
            auto node = std::make_shared<Node>();
            node->deserialize(nodeData, basePath, engine, progress);  // <-- forward
            sceneRoot->addChild(node);
        }
    }

    auto scene = std::make_shared<Scene>();
    scene->setSceneRoot(sceneRoot);
    return scene;
}
```

## Responsibility split

`Scene::deserialize()` is a thin pass-through for the `progress` pointer. It does **not** set `progress->total` — that is the responsibility of the `db` layer (Step 4/5), which has access to the raw JSON before any deserialization begins and can count nodes upfront.
