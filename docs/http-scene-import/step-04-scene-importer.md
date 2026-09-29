# Step 04 — SceneImporter + StageScene programmatic import

## Goal

Main-thread side of the feature: drain the queue, maintain the
`path -> node instance` table (with stale-entry sweep), perform the
replace-or-insert semantics, and apply units/coordinate conversion on a wrapper
node. Reuses the existing glTF import flow (`bg2e::db::loadGltf` +
`insertNewNode` + `newNodeParent`) instead of creating a new one.

## StageScene change

### Modify: `apps/bg2e_composer/src/StageScene.hpp`

Add a programmatic overload next to the interactive one:

```cpp
    // Import a glTF file as a sub scene. The loaded node tree hangs from the
    // primary selected node, or from the editable root if nothing is selected.
    void importGltfScene(const std::filesystem::path& path);

    // Programmatic variant for the HTTP import service. Wraps the loaded tree
    // in a node that carries the unit scaling and, for zUp sources, a -90 deg
    // X rotation. Returns the wrapper node (the instance tracked by the import
    // table), or nullptr on failure with the reason in errorOut. Never shows
    // UI dialogs.
    std::shared_ptr<bg2e::scene::Node> importGltfScene(
        const std::filesystem::path& path,
        float unitsScale,
        bool sourceIsZUp,
        std::string& errorOut);

    // Removes a node previously returned by the programmatic import without
    // asking for confirmation (used for import replacement). Deselects first
    // if the node (or a descendant) is selected.
    void removeImportedNode(std::shared_ptr<bg2e::scene::Node> node);
```

### Modify: `apps/bg2e_composer/src/StageScene.cpp`

```cpp
std::shared_ptr<bg2e::scene::Node> StageScene::importGltfScene(
    const std::filesystem::path& path,
    float unitsScale,
    bool sourceIsZUp,
    std::string& errorOut)
{
    try
    {
        std::shared_ptr<bg2e::scene::Node> loaded(bg2e::db::loadGltf(path, _engine));
        if (!loaded)
        {
            errorOut = "Could not load the specified glTF file.";
            return nullptr;
        }

        // Wrapper node: the registered instance. Carries the unit scale and
        // axis conversion so the imported subtree stays untouched.
        auto wrapperName = path.stem().string();
        auto wrapper = std::make_shared<bg2e::scene::Node>(wrapperName);

        auto transform = new bg2e::scene::TransformComponent();
        glm::mat4 m { 1.0f };
        if (sourceIsZUp)
        {
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });
        }
        if (unitsScale != 1.0f)
        {
            m = glm::scale(m, glm::vec3{ unitsScale });
        }
        transform->setMatrix(m);   // or setScale + rotate; use the existing API
        wrapper->addComponent(transform);

        wrapper->addChild(loaded);

        insertNewNode(wrapper, newNodeParent());   // selection-aware, safe update
        return wrapper;
    }
    catch (const std::exception& error)
    {
        errorOut = error.what();
        return nullptr;
    }
}
```

```cpp
void StageScene::removeImportedNode(std::shared_ptr<bg2e::scene::Node> node)
{
    if (!node) return;
    auto parent = node->parent();
    if (!parent) return;    // already detached (e.g. user removed it)

    // Same safety rule as removeSelectedNode(): the SelectionManager keeps
    // weak references; never remove a still-selected node.
    auto selected = _appDelegate->selectionManager()->selectedNode();
    if (selected && selected->shared_from_this() == node)   // plus descendant check if needed
    {
        _appDelegate->selectionManager()->deselect();
    }

    bg2e::app::MainLoop::current()->safeUpdateScene([this, node, parent]() {
        parent->removeChild(node);          // sets node->_parent = nullptr
        _containerRoot->scene()->updateAll();
    });
    _document->setUnsavedChanges(true);
}
```

Notes:

- The existing interactive `importGltfScene(path)` keeps its MessageBox-based
  UX; optionally it can delegate to the overload with `unitsScale=1, zUp=false`
  and re-show the dialog on error. Minimal change preferred: leave it as-is.
- Wrapper node also solves naming: `insertNewNode` adds Selectable/Gizmo
  components to the wrapper, and the user renames the wrapper freely — the
  table tracks the instance, not the name.

## SceneImporter

### Create: `apps/bg2e_composer/src/SceneImporter.hpp`

```cpp
#pragma once

#include "ImportRequest.hpp"

#include <bg2e/scene/Node.hpp>

#include <memory>
#include <string>
#include <unordered_map>

class StageScene;
class ImportServer;

class SceneImporter {
public:
    explicit SceneImporter(StageScene * stage);

    // Called once per frame from AppDelegate::update() (main thread).
    // Sweeps stale table entries, then drains the server queue.
    void processQueue(ImportServer& server);

    // Drops all entries (scene close/open, app shutdown).
    void clear();

private:
    struct ImportEntry {
        std::weak_ptr<bg2e::scene::Node> node;   // instance identity — never the name
        std::string identifier;                  // node->identifier(), extra validation
    };

    StageScene * _stage;
    std::unordered_map<std::string, ImportEntry> _table;  // key: canonical path

    static std::string tableKey(const std::filesystem::path& p);
    void sweep();
    void processOne(ImportServer& server, const ImportRequest& req);
};
```

### Create: `apps/bg2e_composer/src/SceneImporter.cpp`

```cpp
std::string SceneImporter::tableKey(const std::filesystem::path& p)
{
    // Canonical so the same file reached via different spellings maps to
    // one entry. weakly_canonical tolerates non-existent intermediates.
    return std::filesystem::weakly_canonical(p).string();
}

void SceneImporter::sweep()
{
    for (auto it = _table.begin(); it != _table.end(); )
    {
        auto node = it->second.node.lock();
        // expired  -> node destroyed (user deleted it / scene closed)
        // !parent  -> node detached by Node::removeChild
        if (!node || node->parent() == nullptr ||
            node->identifier() != it->second.identifier)
        {
            it = _table.erase(it);          // prevents dangling entries/leaks
        }
        else
        {
            ++it;
        }
    }
}

void SceneImporter::processQueue(ImportServer& server)
{
    sweep();
    for (const auto& req : server.popPending())
    {
        processOne(server, req);
    }
}

void SceneImporter::processOne(ImportServer& server, const ImportRequest& req)
{
    const auto key = tableKey(req.filePath);

    // Replacement semantics: known path whose node is still alive ->
    // load the new version FIRST (keep the old one on failure), then swap.
    auto it = _table.find(key);
    std::shared_ptr<bg2e::scene::Node> oldNode;
    if (it != _table.end())
    {
        oldNode = it->second.node.lock();
        if (oldNode && oldNode->parent() == nullptr) oldNode.reset();  // detached
    }

    std::string error;
    auto newNode = _stage->importGltfScene(
        req.filePath, req.unitsScale,
        req.coordinateSystem == ImportCoordinateSystem::ZUp, error);

    if (!newNode)
    {
        server.fulfil(req.id, false, error);
        return;
    }

    if (oldNode)
    {
        _stage->removeImportedNode(oldNode);
    }

    _table[key] = ImportEntry{ newNode, newNode->identifier() };
    server.fulfil(req.id, true, "Imported " + req.fileName);
}
```

## Execution flow (POST → imported node)

1. Worker thread: validated `ImportRequest` enqueued; handler blocks on condvar.
2. Next frame, main thread: `AppDelegate::update()` → `processQueue()`.
3. `sweep()` purges stale entries (covers user deletions, scene close/open).
4. `processOne()`:
   - first import → `importGltfScene` → wrapper inserted under the selected
     node (or editable root via `newNodeParent()`), registered;
   - replacement → new wrapper inserted, old node removed via
     `removeImportedNode`, table updated.
5. `fulfil()` wakes the handler → HTTP 200/500.

`insertNewNode` internally defers the actual `addChild` to
`MainLoop::safeUpdateScene` (`device().waitIdle()` at frame top), so scene/GPU
safety matches the existing interactive import exactly.

## Verification

- Covered by the Postman matrix in step 07 (stages 3–7), including:
  import → rename → re-import (replacement), delete node → re-import (fresh
  insert), and sweep after `File > Open Scene` (all entries gone).
