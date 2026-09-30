/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "SceneImporter.hpp"

#include "ImportServer.hpp"
#include "StageScene.hpp"
#include "AppDelegate.hpp"

#include <exception>

SceneImporter::SceneImporter(StageScene * stage, AppDelegate * appDelegate) :
    _stage(stage), _appDelegate(appDelegate)
{
}

std::string SceneImporter::tableKey(const std::filesystem::path& path)
{
    return std::filesystem::weakly_canonical(path).string();
}

void SceneImporter::sweep()
{
    for (auto it = _table.begin(); it != _table.end(); )
    {
        auto node = it->second.node.lock();
        if (!node || node->parent() == nullptr ||
            node->identifier() != it->second.identifier)
        {
            it = _table.erase(it);
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
    ImportRequest request;
    if (server.takePending(request)) processOne(server, request);
}

void SceneImporter::processOne(ImportServer& server, const ImportRequest& request)
{
    const auto key = tableKey(request.filePath);

    auto it = _table.find(key);
    std::shared_ptr<bg2e::scene::Node> oldNode;
    std::shared_ptr<bg2e::scene::Node> oldParent;
    if (it != _table.end())
    {
        oldNode = it->second.node.lock();
        if (oldNode)
        {
            if (auto* parent = oldNode->parent()) oldParent = parent->shared_from_this();
            else oldNode.reset();
        }
    }

    _appDelegate->selectionManager()->deselect();
    struct Outcome {
        std::shared_ptr<bg2e::scene::Node> node;
        std::string error;
    };
    auto outcome = std::make_shared<Outcome>();
    _appDelegate->asyncLoadGuarded(
        [this, request, oldNode, oldParent, outcome](bg2e::ui::Loader* loader) {
            outcome->node = _stage->importGltfScene(
                request.filePath, request.unitsScale,
                request.coordinateSystem == ImportCoordinateSystem::ZUp,
                outcome->error, oldParent,
                [loader](const std::string& name, int processed, int total) {
                    loader->setMessage("Importing " + name + "...");
                    loader->setProgress(total > 0 ? static_cast<float>(processed) / total : 0.0f);
                });
            if (outcome->node && oldNode) _stage->removeImportedNode(oldNode);
        },
        glm::vec4{ 0.2, 0.2, 0.31, 1.0f },
        [this, &server, request, key, outcome](std::exception_ptr exception) {
            if (exception)
            {
                try { std::rethrow_exception(exception); }
                catch (const std::exception& e) { outcome->error = e.what(); }
                catch (...) { outcome->error = "Unknown import error"; }
            }
            if (outcome->node && outcome->node->parent())
            {
                _table[key] = ImportEntry{ outcome->node, outcome->node->identifier() };
                server.fulfil(request.id, true, "Imported " + request.fileName);
            }
            else
            {
                server.fulfil(request.id, false,
                    outcome->error.empty() ? "Import failed" : outcome->error);
            }
        }
    );
}

void SceneImporter::clear()
{
    _table.clear();
}
