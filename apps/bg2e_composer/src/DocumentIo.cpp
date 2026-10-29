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
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/db/scene.hpp>

#include <cmath>
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <bg2e/db/scene_path.hpp>

namespace bg2e_composer {

void AppDelegate::asyncLoadGuarded(std::function<void(bg2e::ui::Loader*)> loadFn,
    glm::vec4 clearColor, std::function<void(std::exception_ptr)> onComplete)
{
    if (sceneLoading())
    {
        throw std::logic_error("A scene load is already running");
    }
    _asyncLoadInProgress = true;
    try
    {
        bg2e::app::MainLoop::current()->asyncLoad(std::move(loadFn), clearColor,
            [weak = weak_from_this(), complete = std::move(onComplete)](std::exception_ptr error)
            {
                if (auto owner = weak.lock())
                {
                    owner->_asyncLoadInProgress = false;
                    if (complete)
                    {
                        complete(error);
                    }
                }
            });
    }
    catch (...)
    {
        _asyncLoadInProgress = false;
        throw;
    }
}

void AppDelegate::newDocument()
{
    if (!_stage || _pendingIo || sceneLoading())
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
    if (sceneLoading())
    {
        return;
    }
    ImportRequest request;
    request.filePath = path;
    request.fileName = path.filename().string();
    request.unitsScale = unitsScale;
    request.coordinateSystem = sourceIsZUp ? ImportCoordinateSystem::ZUp : ImportCoordinateSystem::YUp;
    request.documentGeneration = document() ? document()->generation : 0;
    enqueueImport(std::move(request));
}

void AppDelegate::exportSelectedModels()
{
    if (!_stage || !_selection || _pendingIo || sceneLoading())
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
    const bool saved = _stage->save(path, document()->savePath.empty());
    sampleNavigation(false);
    return saved; // Cancellation/failure prevents replacement.
}

void AppDelegate::processDocumentIo()
{
    if (!_pendingIo || !_stage || _asyncLoadInProgress)
    {
        return;
    }
    auto operation = std::move(*_pendingIo);
    _pendingIo.reset();
    std::shared_ptr<bg2e::scene::Node> candidate;
    bool published = false;
    try
    {
        stopLightmapBake(true);
        using Kind = PendingIo::Kind;
        const bool replacing = operation.kind == Kind::Open || operation.kind == Kind::New;
        if (!flushDocumentCommands(true, replacing))
        {
            return;
        }
        _engine->device()->waitIdle();
        if (operation.kind == Kind::Save)
        {
            const bool saved = _stage->save(operation.path, operation.destinationConfirmed);
            sampleNavigation(false);
            _status = saved ? "Saved " + document()->savePath.filename().string() : "Scene save cancelled or failed (see log)";
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
            _status = _stage->exportModels(targets, operation.path, operation.batch) ?
                "Exported selected models" : "Model export failed (see log)";
            return;
        }
        // Do not allocate GPU resources for a replacement before the dirty prompt.
        if (!operation.replacementConfirmed && !saveBeforeReplace())
        {
            _status = "Scene replacement cancelled";
            return;
        }
        if (operation.kind == Kind::Open && !operation.schema)
        {
            // Read/parse detached JSON on the worker. Component factories, GPU
            // realization and graph publication remain at the stopped main boundary.
            auto path = std::filesystem::absolute(operation.path).lexically_normal();
            if (std::filesystem::is_directory(path))
            {
                path = bg2e::db::resolveSceneLoadPath(path);
            }
            auto extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            if (extension == ".json" || extension == ".bg2scene" || extension == ".bg2e" || extension == ".vitscnj")
            {
                cancelImports();
                operation.path = path;
                operation.generation = document()->generation;
                operation.replacementConfirmed = true;
                auto schema = std::make_shared<std::shared_ptr<bg2e::json::JsonNode>>();
                asyncLoadGuarded([path, schema](bg2e::ui::Loader* loader)
                {
                    loader->setProgress(0);
                    loader->setMessage("Reading scene: " + path.filename().string());
                    *schema = bg2e::db::readSceneDocument(path);
                    loader->setProgress(1);
                    loader->setMessage("Preparing scene resources...");
                }, {0, 0, 0, 1},
                [weak = weak_from_this(), operation = std::move(operation), schema](std::exception_ptr error) mutable
                {
                    auto owner = weak.lock();
                    if (!owner)
                    {
                        return;
                    }
                    try
                    {
                        if (error)
                        {
                            std::rethrow_exception(error);
                        }
                        if (!owner->document() || owner->document()->generation != operation.generation)
                        {
                            throw std::runtime_error("Scene load belongs to a replaced document");
                        }
                        operation.schema = std::move(*schema);
                        owner->_pendingIo = std::move(operation);
                        owner->_sceneResourceLoad = true;
                        owner->_sceneLoader.setProgress(0);
                        owner->_sceneLoader.setMessage("Preparing scene resources...");
                        bg2e::app::MainLoop::current()->pauseScene();
                        owner->_status = "Preparing scene resources...";
                    }
                    catch (const std::exception& failure)
                    {
                        owner->_status = std::string("Scene operation failed: ") + failure.what();
                        bg2e::app::MessageBox::showWarning("Scene operation failed", owner->_status,
                            {{0, "OK", bg2e::app::MessageBox::Return}});
                    }
                    owner->requestSceneFrame();
                });
                return;
            }
        }
        if (operation.schema)
        {
            candidate = operation.candidate;
            if (!candidate)
            {
                candidate = _stage->load(operation.path, operation.schema, false);
                operation.candidate = candidate;
                std::function<void(const std::shared_ptr<bg2e::scene::Node>&)> collect;
                collect = [&](const auto& node)
                {
                    for (const auto& component : node->orderedComponents())
                    {
                        operation.resources.push_back({component, node->name()});
                    }
                    for (const auto& child : node->children())
                    {
                        collect(child);
                    }
                };
                collect(candidate);
            }
            if (operation.nextResource < operation.resources.size())
            {
                const auto& resource = operation.resources[operation.nextResource];
                _sceneLoader.setMessage("Loading " + resource.nodeName + "...");
                resource.component->loadResources(*_engine);
                ++operation.nextResource;
                _sceneLoader.setProgress(static_cast<float>(operation.nextResource) / operation.resources.size());
                _pendingIo = std::move(operation);
                requestSceneFrame();
                bg2e::app::MainLoop::current()->requestFrame();
                return; // Present the Loader between individual component uploads.
            }
        }
        else
        {
            candidate = operation.kind == Kind::New ? _stage->newDocumentRoot() : _stage->load(operation.path);
        }
        _stage->prepareTools(candidate);
        // Prepare all command ownership/callback storage before publication.
        auto commands = makeCommandDispatcher(document()->generation + 1);
        auto previousCommands = document()->commands;
        // Snapshot selected identities before tentative publication. The old
        // document remains owned by StageScene until reattachment succeeds.
        std::vector<bg2e::manipulation::SelectionItem> previousSelection;
        for (const auto& item : _selection->selectedItems())
        {
            previousSelection.push_back(*item);
        }
        if (auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(renderer());
            deferred && deferred->environmentGenerator())
        {
            deferred->environmentGenerator()->resetSourceCache();
        }
        _stage->adopt(candidate, operation.kind == Kind::New ? std::filesystem::path{} : operation.path,
            std::move(commands), [&](bool restoring)
            {
                _selection->deselect();
                if (restoring)
                {
                    for (const auto& item : previousSelection)
                    {
                        if (auto node = item.node.lock(); node && _stage->contains(node.get()))
                        {
                            if (auto mesh = item.lockMesh(); mesh && node->drawable())
                            {
                                _selection->addToSelectedItems(node.get(), node->drawable(), item.submesh);
                            }
                            else
                            {
                                _selection->addNodeToSelection(node.get());
                            }
                        }
                    }
                }
                setScene(scene());
                // Commit extraction now, so a failed attach restores the old
                // graph AND renderer before the old history can be retired.
                DefaultRenderLoopDelegate::prepareFrame();
            });
        published = true;
        if (previousCommands)
        {
            previousCommands->shutdown();
            previousCommands.reset();
        }
        invalidateImports();
        for (int button = 0; button < 3; ++button)
        {
            _input.mouseButtonUp(scene().get(), button, 0, 0);
        }
        _clipboard.reset();
        _viewpointStatus.clear();
        invalidateLightmapSources();
        // The new document owns its EnvironmentComponent, but environment
        // generation runs at Renderer::prepareFrame's stopped boundary.
        requestSceneFrame();
        _lastEnvironmentError.clear(); // A failed HDR on this document must be reported again.
        _status = operation.kind == Kind::New ? "New scene" : "Opened " + operation.path.filename().string();
        if (_sceneResourceLoad)
        {
            _sceneResourceLoad = false;
            bg2e::app::MainLoop::current()->resumeScene();
        }
    }
    catch (const std::exception& error)
    {
        if (!commandFaulted())
        {
            _stage->discardCandidate(candidate);
        }
        if (_sceneResourceLoad)
        {
            _sceneResourceLoad = false;
            bg2e::app::MainLoop::current()->resumeScene();
        }
        _status = std::string(published ? "Scene opened; follow-up update failed: " : "Scene operation failed: ") + error.what();
        bg2e::app::MessageBox::showWarning("Scene operation failed", _status,
            {{0, "OK", bg2e::app::MessageBox::Return}});
        requestSceneFrame();
    }
}

}
