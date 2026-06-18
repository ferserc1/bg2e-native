# Step 5 — Public API in `db/scene.hpp` and `db/scene.cpp`

## Files

- `lib/include/bg2e/db/scene.hpp`
- `lib/src/bg2e/db/scene.cpp`

---

## Header changes (`db/scene.hpp`)

Add `onProgress = nullptr` as the last parameter on all four functions. Existing call sites require no changes.

```cpp
#include <bg2e/scene/Node.hpp>  // for SceneProgressCallback

extern BG2E_API std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& filePath,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress = nullptr);

extern BG2E_API std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress = nullptr);

extern BG2E_API void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& filePath,
    bg2e::scene::SceneProgressCallback onProgress = nullptr);

extern BG2E_API void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::scene::SceneProgressCallback onProgress = nullptr);
```

---

## Implementation changes (`db/scene.cpp`)

### `loadScene` (primary overload — takes `filePath`)

```cpp
std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& filePath,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress)
{
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        bg2e_log_error << "Could not open scene file at path \"" << filePath << "\""
                       << bg2e_log_end;
        return nullptr;
    }

    inFile.seekg(0, std::ios::end);
    std::string content;
    content.resize(inFile.tellg());
    inFile.seekg(0, std::ios::beg);
    inFile.read(&content[0], content.size());

    auto parser = json::JsonParser(content);
    auto sceneFile = parser.parse();

    if (!sceneFile) {
        bg2e_log_error << "Error parsing scene file at path \"" << filePath << "\""
                       << bg2e_log_end;
        return nullptr;
    }

    // Build progress context and pre-count nodes
    scene::SceneLoadProgress progress;
    if (onProgress) {
        progress.callback = onProgress;
        auto& obj = sceneFile->objectValue();
        if (obj.count("scene") && obj["scene"]->isList()) {
            for (auto& nodeData : obj["scene"]->listValue()) {
                progress.total += countJsonNodes(nodeData);
            }
        }
    }

    auto scene = scene::Scene::deserialize(
        sceneFile,
        filePath.parent_path(),
        engine,
        onProgress ? &progress : nullptr);

    inFile.close();

    if (!scene) {
        bg2e_log_error << "Error loading scene content from file \"" << filePath << "\". "
                       << "This error is usually caused by issues with the scene configuration "
                       << "or missing resources." << bg2e_log_end;
    }

    return scene;
}
```

### `loadScene` (secondary overload — takes `basePath + fileName`)

```cpp
std::shared_ptr<bg2e::scene::Scene> loadScene(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::render::Engine& engine,
    bg2e::scene::SceneProgressCallback onProgress)
{
    return loadScene(basePath / fileName, engine, onProgress);
}
```

### `saveScene` (primary overload — takes `filePath`)

```cpp
void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& filePath,
    bg2e::scene::SceneProgressCallback onProgress)
{
    auto rootPath = filePath;
    rootPath.remove_filename();

    bg2e::scene::DrawableRegistry registry;
    bg2e::scene::FindNodeComponentVisitor<bg2e::scene::DrawableComponent> findDrawables;
    auto drawableNodes = findDrawables.find(sceneRoot);
    for (auto& weakNode : drawableNodes) {
        if (auto node = weakNode.lock()) {
            auto comp = node->getComponent<bg2e::scene::DrawableComponent>();
            if (comp && comp->drawableBase()) {
                registry.registerDrawable(comp->drawableBase());
            }
        }
    }

    // Build progress context and pre-count nodes
    scene::SceneSaveProgress progress;
    if (onProgress) {
        progress.callback = onProgress;
        progress.total = countSceneNodes(sceneRoot);
    }

    auto sceneData = sceneRoot->serialize(rootPath, onProgress ? &progress : nullptr);

    std::ofstream file;
    file.open(filePath);
    if (file.is_open()) {
        using namespace bg2e::json;
        auto sceneJson = JSON(JsonObject{
            { "fileType", JSON("bg2e::scene") },
            { "version", JSON(JsonObject{
                { "major", JSON(1) },
                { "minor", JSON(0) },
                { "rev",   JSON(0) },
            })},
            { "scene", JSON(JsonList{sceneData}) }
        });
        file << sceneJson->toString();
        file.close();
    }

    registry.cleanup();
}
```

### `saveScene` (secondary overload — takes `basePath + fileName`)

```cpp
void saveScene(
    bg2e::scene::Node* sceneRoot,
    const std::filesystem::path& basePath,
    const std::string& fileName,
    bg2e::scene::SceneProgressCallback onProgress)
{
    saveScene(sceneRoot, basePath / fileName, onProgress);
}
```

> **Note:** The original secondary overload used `basePath += fileName` (string concatenation) instead of `basePath / fileName` (path join). This step corrects that bug while the signatures are being updated.

---

## Example usage (caller side)

```cpp
auto scene = bg2e::db::loadScene(path, engine, [](const std::string& name, int loaded, int total) {
    float pct = total > 0 ? (100.0f * loaded / total) : 0.0f;
    std::cout << std::format("[{:.0f}%] loading '{}' ({}/{})\n", pct, name, loaded, total);
});
```
