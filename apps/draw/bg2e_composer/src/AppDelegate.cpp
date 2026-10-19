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
#include <bg2e/draw/FsrUpscaler.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/pipeline/SMAAProcessor.hpp>
#include <bg2e/utils/ViewpointAnalyzer.hpp>

#include <exception>
#include <algorithm>
#include <bg2e/manipulation/GizmoComponent.hpp>

namespace bg2e_composer {

AppDelegate::AppDelegate()
    : AppDelegate(std::make_shared<bg2e::manipulation::GizmoAndSelectionRenderer>())
{
}

AppDelegate::AppDelegate(std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer> overlay)
    : DefaultRenderLoopDelegate(std::make_unique<bg2e::draw::RendererDeferred>(), configuration(overlay)),
      _overlay(std::move(overlay))
{
}

bg2e::draw::RendererConfiguration AppDelegate::configuration(
    const std::shared_ptr<bg2e::manipulation::GizmoAndSelectionRenderer>& overlay)
{
    bg2e::draw::RendererConfiguration result;
    result.overlays.push_back(overlay);
    result.resolution = std::make_shared<bg2e::draw::ResolutionSettings>();
    result.upscaler = std::make_shared<bg2e::draw::FsrUpscaler>();
    std::static_pointer_cast<bg2e::draw::FsrUpscaler>(result.upscaler)->setEnabled(false);
    result.finalProcessor = std::make_shared<bg2e::draw::pipeline::SMAAProcessor>();
    return result;
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::createScene()
{
    _stage = std::make_shared<StageScene>(_engine);
    auto root = _stage->create();
    _selection = std::make_shared<bg2e::manipulation::SelectionManager>(_engine);
    _selection->init();
    const auto weakSelection = std::weak_ptr<bg2e::manipulation::SelectionManager>(_selection);
    const auto weakStage = std::weak_ptr<StageScene>(_stage);
    const auto weak = weak_from_this();
    _selection->onSelect([weak, weakSelection, weakStage]
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        if (owner->_selectionChanged)
        {
            owner->_selectionChanged();
        }
        auto selection = weakSelection.lock();
        auto stage = weakStage.lock();
        auto node = selection ? selection->selectedNode() : nullptr;
        const bool lightSelected = node && node->light() && stage &&
            (stage->contains(node) || node == stage->fillLightNode().get());
        bg2e::manipulation::GizmoComponent::setCurrentTransform(lightSelected ? node : nullptr);
        if (lightSelected)
        {
            if (auto gizmo = node->getComponent<bg2e::manipulation::GizmoComponent>())
            {
                gizmo->synchronizeResources();
            }
        }
    });
    return root;
}

void AppDelegate::setLogicalExtent(uint32_t width, uint32_t height)
{
    _logicalExtent = {width, height};
}

glm::ivec2 AppDelegate::inputPosition(int x, int y) const
{
    if (_stage && _stage->camera() && _logicalExtent.x && _logicalExtent.y)
    {
        const auto viewport = _stage->camera()->projection()->viewport();
        return {int(x * viewport.width / _logicalExtent.x), int(y * viewport.height / _logicalExtent.y)};
    }
    return {x, y};
}

void AppDelegate::queueInput(std::function<void()> event)
{
    const auto generation = document() ? document()->generation : 0;
    enqueueRendererUpdate(rendererGeneration(), [this, weak = weak_from_this(), generation, event = std::move(event)](auto& renderer)
    {
        auto owner = weak.lock();
        if (!owner)
        {
            return;
        }
        if (!document() || document()->generation != generation)
        {
            return;
        }
        const bool manipulating = _selection && _selection->sceneInputBlocked();
        try
        {
            _stage->camera()->update(0);
            event(); // All picking/manipulation runs before command acquisition.
        }
        catch (const std::exception& error)
        {
            _status = error.what();
        }
        if (manipulating)
        {
            _stage->markModified();
        }
        renderer.invalidateHistories();
    });
}

void AppDelegate::mouseMove(int x, int y)
{
    queueInput([this, x, y]
    {
        const auto p = inputPosition(x, y);
        const bool manipulating = _selection->sceneInputBlocked();
        if (_selection->mouseMove(_stage->sceneObject(), p.x, p.y))
        {
            _input.mouseMove(scene().get(), x, y);
        }
        if (manipulating)
        {
            _stage->markModified();
        }
    });
}

void AppDelegate::mouseButtonDown(int button, int x, int y)
{
    queueInput([this, button, x, y]
    {
        const auto p = inputPosition(x, y);
        if (_selection->mouseButtonDown(_stage->sceneObject(), button, p.x, p.y))
        {
            _input.mouseButtonDown(scene().get(), button, x, y);
        }
    });
}

void AppDelegate::mouseButtonUp(int button, int x, int y)
{
    queueInput([this, button, x, y]
    {
        const auto p = inputPosition(x, y);
        if (_selection->mouseButtonUp(_stage->sceneObject(), button, p.x, p.y, _additiveSelection))
        {
            _input.mouseButtonUp(scene().get(), button, x, y);
        }
    });
}

void AppDelegate::mouseWheel(int dx, int dy)
{
    queueInput([this, dx, dy]
    {
        if (!_selection->sceneInputBlocked())
        {
            _input.mouseWheel(scene().get(), dx, dy);
        }
    });
}

void AppDelegate::resetCamera()
{
    const auto weakStage = std::weak_ptr<StageScene>(_stage);
    enqueueRendererUpdate(rendererGeneration(), [weakStage](bg2e::draw::Renderer& renderer)
    {
        if (auto stage = weakStage.lock())
        {
            stage->resetCamera();
            renderer.invalidateHistories();
        }
    });
}

void AppDelegate::frameSelection()
{
    if (!_stage || !_selection || _pendingIo)
    {
        return;
    }
    std::shared_ptr<bg2e::scene::Node> target;
    if (auto* selected = _selection->selectedNode())
    {
        target = selected->shared_from_this();
    }
    if (!target || !_stage->contains(target.get()))
    {
        target = _stage->document().root;
    }
    if (!target)
    {
        return;
    }
    const auto weak = weak_from_this();
    const auto weakTarget = std::weak_ptr<bg2e::scene::Node>(target);
    const uint64_t generation = _stage->document().generation;
    enqueueRendererUpdate(rendererGeneration(), [weak, weakTarget, generation](bg2e::draw::Renderer& renderer)
    {
        auto owner = weak.lock();
        auto node = weakTarget.lock();
        if (!owner || !node || !owner->_stage || owner->_stage->document().generation != generation ||
            !owner->_stage->contains(node.get()))
        {
            return;
        }
        if (!owner->_stage->frameNode(node.get()))
        {
            owner->_viewpointStatus = "Selected model has no visible geometry to frame";
            return;
        }
        owner->_viewpointStatus = "Camera framed " + node->name();
        renderer.invalidateHistories();
    });
}

void AppDelegate::analyzeBestView()
{
    if (!_stage || !_selection || _pendingIo || !_engine || !_engine->device())
    {
        return;
    }
    std::shared_ptr<bg2e::scene::Node> target;
    if (auto* selected = _selection->selectedNode())
    {
        target = selected->shared_from_this();
    }
    if (!target || !_stage->contains(target.get()))
    {
        target = _stage->document().root;
    }
    if (!target)
    {
        return;
    }
    const auto weak = weak_from_this();
    const auto weakTarget = std::weak_ptr<bg2e::scene::Node>(target);
    const uint64_t generation = _stage->document().generation;
    enqueueRendererUpdate(rendererGeneration(), [weak, weakTarget, generation](bg2e::draw::Renderer& renderer)
    {
        auto owner = weak.lock();
        auto node = weakTarget.lock();
        if (!owner || !node || !owner->_stage || owner->_stage->document().generation != generation ||
            !owner->_stage->contains(node.get()))
        {
            return;
        }
        try
        {
            owner->_viewpointStatus = "Analyzing camera views...";
            owner->_engine->device()->waitIdle();
            auto samples = owner->_stage->analyzeBestView(node.get());
            const auto* best = bg2e::utils::ViewpointAnalyzer::bestSample(samples);
            if (!best || best->analysis.visiblePixelCount == 0)
            {
                owner->_viewpointStatus = "Camera analysis found no visible model geometry";
                return;
            }
            const auto candidate = best->camera;
            const auto analysis = best->analysis;
            const float score = best->score;
            const size_t bestIndex = static_cast<size_t>(best - samples.data());
            std::vector<size_t> visible;
            for (size_t index = 0; index < analysis.pixelsPerSubmesh.size(); ++index)
            {
                if (analysis.pixelsPerSubmesh[index] > 0)
                {
                    visible.push_back(index);
                }
            }
            std::sort(visible.begin(), visible.end(), [&](size_t lhs, size_t rhs)
            {
                return analysis.pixelsPerSubmesh[lhs] > analysis.pixelsPerSubmesh[rhs];
            });
            owner->_stage->applyViewpoint(candidate);
            owner->_viewpointStatus = "Best of " + std::to_string(samples.size()) + " views (#" +
                std::to_string(bestIndex + 1) + "): " + std::to_string(analysis.visibleSubmeshCount) +
                "/" + std::to_string(owner->_stage->analyzedSubmeshes().size()) + " submeshes, " +
                std::to_string(analysis.visiblePixelCount) + " pixels, score " + std::to_string(score);
            const auto& submeshes = owner->_stage->analyzedSubmeshes();
            for (size_t rank = 0; rank < std::min<size_t>(visible.size(), 3); ++rank)
            {
                const size_t resultIndex = visible[rank];
                const uint32_t identifier = static_cast<uint32_t>(resultIndex + 1);
                if (identifier == 0 || identifier > submeshes.size() || submeshes[identifier - 1].identifier != identifier)
                {
                    continue;
                }
                const auto node = submeshes[identifier - 1].node.lock();
                if (node)
                {
                    owner->_viewpointStatus += "\n" + node->name() + " / submesh " +
                        std::to_string(submeshes[identifier - 1].submesh) + ": " +
                        std::to_string(analysis.pixelsPerSubmesh[resultIndex]) + " pixels";
                }
            }
            renderer.invalidateHistories();
        }
        catch (const std::exception& error)
        {
            owner->_viewpointStatus = std::string("Camera analysis failed: ") + error.what();
        }
    });
}

std::filesystem::path AppDelegate::chooseSavePath()
{
    auto path = bg2e::app::FileDialog::getSaveFilePath({{"Scene document", "json,bg2scene,bg2e"}, {"Single BG2 model", "bg2"}});
    if (!path.empty() && path.extension().empty())
    {
        path.replace_extension(".json");
    }
    return path;
}

void AppDelegate::requestOpen(const std::filesystem::path& path)
{
    if (!_stage || path.empty() || _pendingIo)
    {
        return;
    }
    PendingIo operation;
    operation.open = path;
    // Dirty prompts run after queued edits are flushed at the stopped boundary.
    _pendingIo = std::move(operation);
    requestSceneFrame();
}

void AppDelegate::openDocument()
{
    if (_pendingIo)
    {
        return;
    }
    const auto path = bg2e::app::FileDialog::getOpenFilePath({{"Models and scenes", "bg2,vwglb,obj,gltf,glb,json,bg2scene,bg2e"}});
    requestOpen(path);
}

void AppDelegate::openDocument(const std::filesystem::path& path)
{
    requestOpen(path);
}

void AppDelegate::fileDropped(const std::filesystem::path& path)
{
    requestOpen(path);
}

void AppDelegate::saveDocument(bool saveAs)
{
    if (!_stage || !_stage->document().root || _pendingIo)
    {
        return;
    }
    auto path = saveAs || _stage->document().savePath.empty() ? chooseSavePath() : _stage->document().savePath;
    if (path.empty())
    {
        return;
    }
    _pendingIo = PendingIo{{}, std::move(path)};
    requestSceneFrame();
}

bool AppDelegate::confirmClose()
{
    if (_pendingIo)
    {
        return false; // Finish the pending transaction before offering another action.
    }
    if (_stage && !_preparing)
    {
        prepareFrame(); // Flush pending edits before deciding whether close needs a prompt.
    }
    if (!_stage || !_stage->document().dirty)
    {
        return true;
    }
    using Box = bg2e::app::MessageBox;
    const int choice = Box::showWarning("Unsaved document", "Save changes before closing?",
        {{0, "Cancel", Box::Esc}, {1, "Discard"}, {2, "Save", Box::Return}});
    if (choice == 1)
    {
        return true;
    }
    if (choice != 2)
    {
        return false;
    }
    auto path = _stage->document().savePath.empty() ? chooseSavePath() : _stage->document().savePath;
    if (path.empty())
    {
        return false;
    }
    try
    {
        // MainLoop invokes this callback between frames, before command acquisition.
        _engine->device()->waitIdle();
        _stage->save(path);
        return true;
    }
    catch (const std::exception& error)
    {
        _status = std::string("Save failed: ") + error.what();
        return false;
    }
}

void AppDelegate::prepareFrame()
{
    _preparing = true;
    struct ResetFlag {
        bool& flag;
        ~ResetFlag()
        {
            flag = false;
        }
    } reset{_preparing};
    retireBakeFrame();
    if (_pendingIo && _stage)
    {
        stopLightmapBake(true);
        auto operation = std::move(*_pendingIo);
        _pendingIo.reset();
        try
        {
            // Save includes all edits already requested by UI/input; opening must
            // consider their dirty notifications before offering to discard them.
            DefaultRenderLoopDelegate::prepareFrame();
            _engine->device()->waitIdle();
            std::shared_ptr<bg2e::scene::Node> candidate;
            if (!operation.open.empty())
            {
                candidate = _stage->load(operation.open);
                _stage->prepareTools(candidate);
                if (_stage->document().dirty)
                {
                    using Box = bg2e::app::MessageBox;
                    const int choice = Box::showWarning("Unsaved document", "Save changes before opening another file?",
                        {{0, "Cancel", Box::Esc}, {1, "Discard"}, {2, "Save", Box::Return}});
                    if (choice == 2)
                    {
                        operation.save = _stage->document().savePath.empty() ? chooseSavePath() : _stage->document().savePath;
                        if (operation.save.empty())
                        {
                            operation.open.clear();
                        }
                    }
                    else if (choice != 1)
                    {
                        operation.open.clear();
                    }
                }
            }
            if (!operation.save.empty())
            {
                _stage->save(operation.save);
                _status = "Saved " + operation.save.filename().string();
            }
            if (!operation.open.empty())
            {
                _stage->adopt(std::move(candidate), operation.open);
                _selection->deselect();
                for (int button = 0; button < 3; ++button)
                {
                    _input.mouseButtonUp(scene().get(), button, 0, 0);
                }
                setScene(scene()); // Rebuild renderer catalogs and drop all old slot queues.
                _status = "Opened " + operation.open.filename().string();
            }
        }
        catch (const std::exception& error)
        {
            _status = std::string("File operation failed: ") + error.what();
        }
    }
    if (_selection && _stage && _selection->selectedNode() && !_stage->contains(_selection->selectedNode()))
    {
        _selection->deselect();
    }
    DefaultRenderLoopDelegate::prepareFrame();
    if (_selection && _stage)
    {
        _selection->refreshSelection();
        if (_selection->selectedNode() && !_stage->contains(_selection->selectedNode()))
        {
            _selection->deselect();
        }
    }
    if (_stage)
    {
        _stage->releaseRetired(); // Renderer references were released before GPU resource cleanup.
    }
    if (_quitPending)
    {
        _quitPending = false;
        if (confirmClose() && _quit)
        {
            _quit();
        }
    }
}

void AppDelegate::cleanup()
{
    std::exception_ptr error;
    try
    {
        stopLightmapBake(true);
    }
    catch (...)
    {
        error = std::current_exception();
    }
    if (_toolsCleanup)
    {
        try
        {
            _toolsCleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
        _toolsCleanup = {};
    }
    _selectionChanged = {};
    _clipboard.reset();
    bg2e::manipulation::GizmoComponent::setCurrentTransform(nullptr);
    if (_selection)
    {
        _selection->deselect();
        _selection.reset();
    }
    try
    {
        DefaultRenderLoopDelegate::cleanup();
    }
    catch (...)
    {
        if (!error)
        {
            error = std::current_exception();
        }
    }
    if (_stage)
    {
        try
        {
            _stage->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
        _stage.reset();
    }
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
