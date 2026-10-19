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

#include "AppDelegate.hpp"

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/MessageBox.hpp>
#include <bg2e/gpu/Device.hpp>

#include <cmath>
#include <set>
#include <stdexcept>

namespace bg2e_composer {

void AppDelegate::newDocument()
{
    if (!_stage || _pendingIo)
    {
        return;
    }
    PendingIo operation;
    operation.kind = PendingIo::Kind::New;
    _pendingIo = std::move(operation);
    requestSceneFrame();
}

void AppDelegate::importDocument(const std::filesystem::path& path, float unitsScale, bool sourceIsZUp)
{
    if (!_stage || _pendingIo || path.empty())
    {
        return;
    }
    if (!std::isfinite(unitsScale) || unitsScale <= 0)
    {
        _status = "Import units scale must be finite and positive";
        return;
    }
    PendingIo operation;
    operation.kind = PendingIo::Kind::Import;
    operation.path = path;
    operation.generation = document()->generation;
    auto* selected = _selection ? _selection->selectedNode() : nullptr;
    operation.parent = selected && _stage->contains(selected) ?
        selected->shared_from_this() : document()->root;
    operation.unitsScale = unitsScale;
    operation.sourceIsZUp = sourceIsZUp;
    _pendingIo = std::move(operation);
    requestSceneFrame();
}

void AppDelegate::exportSelectedModels()
{
    if (!_stage || !_selection || _pendingIo)
    {
        return;
    }
    PendingIo operation;
    operation.kind = PendingIo::Kind::Export;
    operation.generation = document()->generation;
    std::set<bg2e::scene::Node*> seen;
    for (const auto& weak : _selection->selectedNodes())
    {
        if (auto node = weak.lock(); node && _stage->contains(node.get()) && seen.insert(node.get()).second)
        {
            operation.targets.push_back(node);
        }
    }
    if (operation.targets.empty())
    {
        _status = "Select at least one model node to export";
        return;
    }
    operation.batch = operation.targets.size() > 1;
    operation.path = operation.batch ? bg2e::app::FileDialog::getPickFolderPath() :
        bg2e::app::FileDialog::getSaveFilePath({{"BG2 model", "bg2"}});
    if (operation.path.empty())
    {
        return;
    }
    if (!operation.batch && !operation.path.has_extension())
    {
        operation.path += ".bg2";
    }
    _pendingIo = std::move(operation);
    requestSceneFrame();
}

bool AppDelegate::saveBeforeReplace()
{
    if (!document()->dirty)
    {
        return true;
    }
    using Box = bg2e::app::MessageBox;
    const auto choice = Box::showWarning("Unsaved scene", "Save changes before replacing this scene?",
        {{0, "Cancel", Box::Esc}, {1, "Discard"}, {2, "Save", Box::Return}});
    if (choice == 1)
    {
        return true;
    }
    if (choice != 2)
    {
        return false;
    }
    const auto path = document()->savePath.empty() ? chooseSavePath() : document()->savePath;
    if (path.empty())
    {
        return false;
    }
    return _stage->save(path, document()->savePath.empty()); // Cancellation/failure prevents replacement.
}

void AppDelegate::processDocumentIo()
{
    if (!_pendingIo || !_stage)
    {
        return;
    }
    auto operation = std::move(*_pendingIo);
    _pendingIo.reset();
    std::shared_ptr<bg2e::scene::Node> candidate;
    try
    {
        stopLightmapBake(true);
        DefaultRenderLoopDelegate::prepareFrame(); // Flush authored edits first.
        _engine->device()->waitIdle();
        using Kind = PendingIo::Kind;
        if (operation.kind == Kind::Save)
        {
            const bool saved = _stage->save(operation.path, operation.destinationConfirmed);
            _status = saved ? "Saved " + document()->savePath.filename().string() : "Scene save cancelled";
            return;
        }
        if (operation.kind == Kind::Export)
        {
            if (operation.generation != document()->generation)
            {
                throw std::runtime_error("Export request belongs to a replaced scene");
            }
            std::vector<std::shared_ptr<bg2e::scene::Node>> targets;
            for (const auto& weak : operation.targets)
            {
                auto node = weak.lock();
                if (!node || !_stage->contains(node.get()))
                {
                    throw std::runtime_error("An export target was removed");
                }
                targets.push_back(std::move(node));
            }
            _stage->exportModels(targets, operation.path, operation.batch);
            _status = "Exported selected models";
            return;
        }
        if (operation.kind == Kind::Import)
        {
            auto parent = operation.parent.lock();
            if (operation.generation != document()->generation || !parent || !_stage->contains(parent.get()))
            {
                throw std::runtime_error("Import parent belongs to a removed or replaced scene");
            }
            candidate = _stage->prepareImport(operation.path, operation.unitsScale, operation.sourceIsZUp);
            invalidateLightmapSources();
            _stage->insert(candidate, parent);
            setScene(scene());
            _status = "Imported " + operation.path.filename().string();
            return;
        }
        // Do not allocate GPU resources for a replacement before the dirty prompt.
        if (!saveBeforeReplace())
        {
            _status = "Scene replacement cancelled";
            return;
        }
        candidate = operation.kind == Kind::New ? _stage->newDocumentRoot() : _stage->load(operation.path);
        _stage->prepareTools(candidate);
        _stage->adopt(candidate, operation.kind == Kind::New ? std::filesystem::path{} : operation.path);
        _selection->deselect();
        for (int button = 0; button < 3; ++button)
        {
            _input.mouseButtonUp(scene().get(), button, 0, 0);
        }
        _clipboard.reset();
        _viewpointStatus.clear();
        invalidateLightmapSources();
        setScene(scene()); // Drop renderer queues before releasing retired resources.
        _status = operation.kind == Kind::New ? "New scene" : "Opened " + operation.path.filename().string();
    }
    catch (const std::exception& error)
    {
        _stage->discardCandidate(candidate);
        _status = std::string("Scene operation failed: ") + error.what();
    }
}

}
