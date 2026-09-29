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

SceneImporter::SceneImporter(StageScene * stage) :
    _stage(stage)
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
    for (const auto& request : server.popPending())
    {
        processOne(server, request);
    }
}

void SceneImporter::processOne(ImportServer& server, const ImportRequest& request)
{
    const auto key = tableKey(request.filePath);

    auto it = _table.find(key);
    std::shared_ptr<bg2e::scene::Node> oldNode;
    if (it != _table.end())
    {
        oldNode = it->second.node.lock();
        if (oldNode && oldNode->parent() == nullptr)
        {
            oldNode.reset();
        }
    }

    std::string error;
    auto newNode = _stage->importGltfScene(
        request.filePath,
        request.unitsScale,
        request.coordinateSystem == ImportCoordinateSystem::ZUp,
        error
    );

    if (!newNode)
    {
        server.fulfil(request.id, false, error);
        return;
    }

    if (oldNode)
    {
        _stage->removeImportedNode(oldNode);
    }

    _table[key] = ImportEntry{ newNode, newNode->identifier() };
    server.fulfil(request.id, true, "Imported " + request.fileName);
}

void SceneImporter::clear()
{
    _table.clear();
}
