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

#include <bg2e/gpu/Device.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace bg2e_composer {

void AppDelegate::initializeImports()
{
    try
    {
        _importSettings.load();
        _importServer.setDocumentGeneration(document()->generation);
        if (_importSettings.serviceEnabled())
        {
            const auto error = _importServer.start(_importSettings.port());
            _importStatus = error.empty() ? "Listening on 127.0.0.1:" + std::to_string(_importSettings.port()) : error;
        }
        else
        {
            _importStatus = "Import service stopped";
        }
    }
    catch (const std::exception& error)
    {
        _importStatus = error.what();
    }
}

void AppDelegate::setImportService(bool enabled, const std::string& text)
{
    try
    {
        if (!enabled)
        {
            cancelImports(true);
            _importServer.stop();
            _importSettings.setServiceEnabled(false);
            _importSettings.save();
            _importStatus = "Import service stopped";
            return;
        }
        uint32_t port = 0;
        if (!ImportSettings::parsePort(text, port))
        {
            throw std::invalid_argument("Invalid port. Use a number between 1024 and 49151");
        }
        const auto error = _importServer.start(port);
        if (!error.empty())
        {
            throw std::runtime_error(error);
        }
        _importSettings.setPort(port);
        _importSettings.setServiceEnabled(true);
        _importSettings.save();
        _importStatus = "Listening on 127.0.0.1:" + std::to_string(port);
    }
    catch (const std::exception& error)
    {
        _importStatus = error.what();
    }
    requestSceneFrame();
}

void AppDelegate::setImportPaused(bool paused)
{
    if (_importPaused == paused)
    {
        return;
    }
    if (auto loop = bg2e::app::MainLoop::current())
    {
        if (paused)
        {
            // A release may be consumed by the loader. Clear scene gestures at
            // entry so cancel/failure cannot leave an old drag pressed forever.
            if (_selection)
            {
                _selection->deselect();
            }
            if (auto controller = _stage ? _stage->orbitCamera() : nullptr)
            {
                auto fresh = controller->clone();
                fresh->setPriority(controller->priority());
                fresh->setIgnoreSerialization(controller->ignoreSerialization());
                auto node = controller->ownerNode();
                node->removeComponent(controller->shared_from_this());
                node->addComponent(fresh);
            }
            loop->pauseScene({0.2f, 0.2f, 0.31f, 1.0f});
        }
        else
        {
            loop->resumeScene();
        }
    }
    _importPaused = paused;
}

void AppDelegate::enqueueImport(ImportRequest request)
{
    auto fail = [&](const std::string& message)
    {
        _importStatus = message;
        if (request.id != 0)
        {
            _importServer.fulfil(request.id, false, message);
        }
    };
    if (!_stage || !document() || request.filePath.empty())
    {
        fail("No scene or import source available");
        return;
    }
    if (request.documentGeneration != document()->generation)
    {
        fail("Import request belongs to a replaced scene");
        return;
    }
    if (!std::isfinite(request.unitsScale) || request.unitsScale <= 0)
    {
        fail("Import units scale must be finite and positive");
        return;
    }
    try
    {
        auto job = std::make_shared<SceneImporter::Job>();
        job->request = request;
        job->token = _nextImportToken++;
        job->documentGeneration = document()->generation;
        job->rendererGeneration = rendererGeneration();
        job->rendererIdentity = renderer();
        auto selected = _selection ? _selection->selectedNode() : nullptr;
        job->parent = selected && _stage->contains(selected) ? selected->shared_from_this() : document()->root;
        job->key = std::filesystem::weakly_canonical(request.filePath).string();
        if (request.id != 0)
        {
            auto found = _imported.find(job->key);
            if (found != _imported.end())
            {
                auto previous = found->second.node.lock();
                if (previous && found->second.generation == document()->generation &&
                    _stage->contains(previous.get()) && previous->parent())
                {
                    job->previous = previous;
                    job->replacing = true;
                    job->parent = previous->parent()->shared_from_this();
                }
                else
                {
                    _imported.erase(found);
                }
            }
        }
        if (!_imports.enqueue(job))
        {
            fail("Import queue is full (16 waiting jobs)");
            return;
        }
        _importStatus = "Queued " + request.fileName;
        requestSceneFrame();
    }
    catch (const std::exception& error)
    {
        fail(error.what());
    }
}

void AppDelegate::cancelImports(bool remoteOnly)
{
    for (const auto& job : _imports.cancel(remoteOnly))
    {
        if (job->request.id != 0)
        {
            _importServer.fulfil(job->request.id, false, "Import cancelled");
        }
    }
    ImportRequest pending;
    if (_importServer.takePending(pending))
    {
        _importServer.fulfil(pending.id, false, "Import cancelled");
    }
    _importStatus = "Import cancelled";
    if (!_imports.busy())
    {
        setImportPaused(false);
    }
    requestSceneFrame();
}

void AppDelegate::invalidateImports()
{
    cancelImports();
    _imported.clear();
    _importServer.setDocumentGeneration(document()->generation);
}

void AppDelegate::processImports()
{
    if (!_stage || !_engine)
    {
        return;
    }
    std::erase_if(_imported, [this](const auto& item)
    {
        auto node = item.second.node.lock();
        return !node || item.second.generation != document()->generation || !_stage->contains(node.get());
    });
    ImportRequest pending;
    if (_importServer.takePending(pending))
    {
        enqueueImport(std::move(pending));
    }
    if (auto job = _imports.takeCompleted())
    {
        bool ok = false;
        std::string message;
        std::shared_ptr<bg2e::scene::Node> candidate;
        bool recoveryFailed = false;
        try
        {
            auto parent = job->parent.lock();
            auto previous = job->previous.lock();
            if (job->cancelled.load())
            {
                throw std::runtime_error("Import cancelled");
            }
            if (job->documentGeneration != document()->generation ||
                job->rendererIdentity != renderer() || !parent || !_stage->contains(parent.get()))
            {
                throw std::runtime_error("Import scene, renderer or destination was replaced");
            }
            if (job->replacing && (!previous || !_stage->contains(previous.get()) || previous->parent() != parent.get()))
            {
                throw std::runtime_error("Reimport target was removed or moved");
            }
            if (!job->error.empty())
            {
                throw std::runtime_error(job->error);
            }
            // Parsing held no renderer/catalog targets. Reacquire the current
            // binding generation after ordinary same-renderer scene/resize rebinds.
            job->rendererGeneration = rendererGeneration();
            if (!withRenderer(job->rendererGeneration, [](auto&) {}))
            {
                throw std::runtime_error("Import renderer targets are not bound");
            }
            stopLightmapBake(true);
            _engine->device()->waitIdle();
            job->loader.setMessage("Realizing " + job->request.fileName);
            candidate = _stage->prepareImport(job->request.filePath, job->request.unitsScale,
                job->request.coordinateSystem == ImportCoordinateSystem::ZUp,
                [job](const std::string& name, int processed, int total)
                {
                    job->loader.setMessage("Realizing " + name);
                    job->loader.setProgress(0.4f + 0.5f * (total > 0 ? float(processed) / float(total) : 0));
                }, job->gltf.get(), job->schema, job->bg2.get(), job->obj.get());
            candidate->setName(job->request.fileName.empty() ? job->request.filePath.stem().string() : job->request.fileName);
            // Reserve table storage before publishing any graph mutation.
            if (job->request.id != 0)
            {
                _imported.try_emplace(job->key);
            }
            _stage->publishImport(candidate, parent, previous, [&](bool restoring)
            {
                try
                {
                    _selection->deselect();
                    _clipboard.reset();
                    command::Effects effects;
                    effects.lightmapSources = true;
                    effects.renderHistories = true;
                    effects.sceneReferences = true;
                    applyCommandEffects(*renderer(), effects);
                    DefaultRenderLoopDelegate::prepareFrame(); // Retire old extraction before GPU cleanup.
                }
                catch (...)
                {
                    recoveryFailed = restoring;
                    throw;
                }
            });
            if (job->request.id != 0)
            {
                _imported[job->key] = {candidate, document()->generation};
            }
            job->loader.setProgress(1);
            ok = true;
            message = "Imported " + job->request.fileName;
        }
        catch (const std::exception& error)
        {
            if (recoveryFailed)
            {
                // Neither binding is usable. Keep resources until normal renderer-
                // first teardown rather than rendering with a failed extraction.
                if (document()->commands)
                {
                    document()->commands->history().reportExternalFault();
                    _stage->synchronizeDirty();
                }
                throw;
            }
            if (candidate && !candidate->parent())
            {
                _stage->discardCandidate(candidate);
            }
            message = error.what();
        }
        _importStatus = message;
        _status = message;
        if (job->request.id != 0)
        {
            // Reply only after scene/renderer bindings have been restored.
            setImportPaused(false);
            _importServer.fulfil(job->request.id, ok, message);
        }
    }
    if (_imports.busy() && !_importPaused)
    {
        stopLightmapBake(true);
        setImportPaused(true);
    }
    _imports.startNext(bg2e::app::MainLoop::current());
    setImportPaused(_imports.busy());
}

}
