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
            finishTransformInteraction(false);
            if (auto controller = _stage ? _stage->orbitCamera() : nullptr)
            {
                controller->cancelInput();
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
        if (!flushDocumentCommands(true))
        {
            fail("Settle pending edits before importing");
            return;
        }
        if (auto controller = _stage->orbitCamera())
        {
            controller->cancelInput();
            sampleNavigation(true);
        }
        auto job = std::make_shared<SceneImporter::Job>();
        job->request = request;
        job->token = _nextImportToken++;
        job->documentGeneration = document()->generation;
        job->sourceState = document()->commands->history().documentState();
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
                auto previous = found->second->node.lock();
                if (previous && found->second->generation == document()->generation &&
                    _stage->contains(previous.get()) && previous->parent())
                {
                    job->previous = previous;
                    job->replacing = true;
                    job->parent = previous->parent()->shared_from_this();
                }
                else
                {
                    found->second->node.reset();
                }
            }
        }
        if (!_imports.enqueue(job))
        {
            fail("Import queue is full (16 waiting jobs)");
            return;
        }
        _latestImportTokens[job->key] = job->token;
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
    if (auto job = _publishingImport.lock(); job && (!remoteOnly || job->request.id != 0))
    {
        job->cancelled = true;
    }
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
    _latestImportTokens.clear();
    _importServer.setDocumentGeneration(document()->generation);
}

void AppDelegate::processImports()
{
    if (!_stage || !_engine)
    {
        return;
    }
    if (_importPublicationPending)
    {
        return;
    }
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
        bool publicationQueued = false;
        try
        {
            auto parent = job->parent.lock();
            auto previous = job->previous.lock();
            if (job->cancelled.load())
            {
                throw std::runtime_error("Import cancelled");
            }
            if (job->sourceState != document()->commands->history().documentState() ||
                _latestImportTokens[job->key] != job->token)
            {
                throw std::runtime_error("Import sources or a newer request changed while parsing");
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
            if (std::filesystem::last_write_time(job->request.filePath) != job->sourceModified ||
                std::filesystem::file_size(job->request.filePath) != job->sourceSize)
            {
                throw std::runtime_error("Import source changed before realization");
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
            std::shared_ptr<command::scene::ImportRecord> record;
            if (job->request.id != 0)
            {
                auto& slot = _imported[job->key];
                if (!slot)
                {
                    slot = std::make_shared<command::scene::ImportRecord>();
                    slot->generation = document()->generation;
                }
                record = slot;
            }
            auto placement = std::make_unique<command::scene::EditNodes>(_stage, _selection,
                previous ? command::scene::NodeEdit::Replace : command::scene::NodeEdit::Insert,
                std::vector{candidate}, parent, std::string(previous ? "Reimport " : "Import ") + candidate->name(), previous);
            const auto expected = job->sourceState;
            const auto generation = job->documentGeneration;
            const auto key = job->key;
            const auto token = job->token;
            auto valid = [weak = weak_from_this(), expected, key, token, job = std::weak_ptr<SceneImporter::Job>(job)]
            {
                auto owner = weak.lock();
                auto request = job.lock();
                return request && !request->cancelled.load() && owner && owner->document() && owner->document()->commands &&
                    owner->document()->commands->history().documentState() == expected &&
                    owner->_latestImportTokens[key] == token;
            };
            auto action = std::make_unique<command::scene::ApplyImport>(std::move(placement), record, candidate, valid);
            command::Effects effects;
            effects.lightmapSources = true;
            effects.renderHistories = true;
            effects.selection = true;
            job->gltf.reset();
            job->bg2.reset();
            job->obj.reset();
            job->schema.reset();
            auto completion = [weak = weak_from_this(), job](auto result)
            {
                if (auto owner = weak.lock())
                {
                    if (owner->_publishingImport.lock() == job)
                    {
                        owner->_importPublicationPending = false;
                        owner->_publishingImport.reset();
                    }
                    if (!owner->document() || owner->document()->generation != job->documentGeneration)
                    {
                        if (job->request.id != 0)
                        {
                            owner->_importServer.fulfil(job->request.id, false, "Import document was replaced");
                        }
                        return;
                    }
                    const bool applied = result.status == bg2e::app::CommandStatus::Applied;
                    owner->_status = owner->_importStatus = applied ? "Imported " + job->request.fileName : std::string(result.message);
                    job->loader.setProgress(applied ? 1.0f : 0.0f);
                    owner->setImportPaused(false);
                    if (job->request.id != 0)
                    {
                        owner->_importServer.fulfil(job->request.id, applied, owner->_importStatus);
                    }
                    owner->requestSceneFrame();
                }
            };
            const auto admitted = document()->commands->submit(std::move(action), command::Boundary::Conservative, effects,
                [weak = weak_from_this(), generation]
                {
                    auto owner = weak.lock();
                    return owner && owner->document() && owner->document()->generation == generation;
                }, completion, bg2e::app::CommandOrigin::PreparedResult);
            publicationQueued = true; // Admission failures also complete exactly once.
            if (admitted.accepted())
            {
                _publishingImport = job;
                _importPublicationPending = true;
            }
            document()->commands->pump();
            message = admitted.accepted() ? "Applying prepared import" : std::string(admitted.message);
        }
        catch (const std::exception& error)
        {
            if (candidate && !candidate->parent())
            {
                _stage->discardCandidate(candidate);
            }
            message = error.what();
        }
        _importStatus = message;
        _status = message;
        if (!publicationQueued && job->request.id != 0)
        {
            // Reply only after scene/renderer bindings have been restored.
            setImportPaused(false);
            _importServer.fulfil(job->request.id, ok, message);
        }
    }
    if (_importPublicationPending)
    {
        return;
    }
    if (_imports.busy() && !_importPaused)
    {
        stopLightmapBake(true);
        setImportPaused(true);
    }
    _imports.startNext(bg2e::app::MainLoop::current(), document()->commands->history().documentState());
    setImportPaused(_imports.busy());
}

}
