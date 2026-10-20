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

#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>

#include <exception>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {

DefaultRenderLoopDelegate::DefaultRenderLoopDelegate(std::unique_ptr<Renderer> renderer,
                                                   RendererConfiguration configuration)
{
    setRenderer(std::move(renderer), std::move(configuration));
}

std::shared_ptr<scene::Node> DefaultRenderLoopDelegate::createScene()
{
    return std::make_shared<scene::Node>();
}

void DefaultRenderLoopDelegate::init(Engine* engine)
{
    if (_engine || !engine)
    {
        throw std::logic_error("Default draw delegate requires one initialized engine");
    }
    engine->device();
    RenderLoopDelegate::init(engine);
}

void DefaultRenderLoopDelegate::initScene()
{
    if (!_engine || _sceneReady)
    {
        throw std::logic_error("Default draw scene initialization is out of order");
    }
    _scene = _scenePending ? std::move(_pendingScene) : createScene();
    _scenePending = false;
    _sceneReady = true;
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setRenderer(std::unique_ptr<Renderer> renderer,
                                          RendererConfiguration configuration)
{
    if (!renderer)
    {
        throw std::invalid_argument("Default draw delegate requires a non-null renderer");
    }
    _pendingRenderer = PendingRenderer{std::move(renderer), std::move(configuration)};
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setScene(std::shared_ptr<scene::Node> sceneRoot)
{
    _pendingScene = std::move(sceneRoot);
    _scenePending = true;
    requestSceneFrame();
}

std::shared_ptr<scene::Node> DefaultRenderLoopDelegate::scene() const
{
    return _scene;
}

Renderer* DefaultRenderLoopDelegate::renderer() const
{
    return _renderer.get();
}

uint64_t DefaultRenderLoopDelegate::rendererGeneration() const
{
    return _rendererGeneration;
}

bool DefaultRenderLoopDelegate::withRenderer(uint64_t generation, const std::function<void(Renderer&)>& operation)
{
    if (!_targetsBound || generation != _rendererGeneration || !operation)
    {
        return false;
    }
    operation(*_renderer);
    return true;
}

void DefaultRenderLoopDelegate::setRendererChangeCallbacks(std::function<void()> detach,
                                                         RendererAttachedCallback attach)
{
    if (_targetsBound)
    {
        throw std::logic_error("Configure renderer callbacks before targets are bound");
    }
    _detach = std::move(detach);
    _attach = std::move(attach);
}

void DefaultRenderLoopDelegate::setContinuousRendering(bool enabled)
{
    _continuousRendering = enabled;
    if (enabled)
    {
        requestSceneFrame();
    }
}

bool DefaultRenderLoopDelegate::continuousRendering() const
{
    return _continuousRendering;
}

void DefaultRenderLoopDelegate::detachTargets()
{
    if (_targetsBound)
    {
        base::CpuProfileScope profile("delegate.detachTargetsSettingsBinding");
        _targetsBound = false;
        ++_rendererGeneration; // Invalidate delayed actions before calling user code.
        const auto detach = _detach;
        base::CpuProfileScope::count("delegate.settingsDetachCallbacks", detach ? 1 : 0);
        if (detach)
        {
            detach();
        }
    }
}

void DefaultRenderLoopDelegate::attachTargets()
{
    base::CpuProfileScope profile("delegate.attachTargetsSettingsBinding");
    base::CpuProfileScope::count("delegate.settingsAttachCallbacks", _attach ? 1 : 0);
    ++_rendererGeneration;
    _targetsBound = true;
    const auto attach = _attach;
    if (attach)
    {
        attach(*_renderer, _rendererGeneration);
    }
}

void DefaultRenderLoopDelegate::releaseRenderer()
{
    std::exception_ptr error;
    try
    {
        detachTargets();
    }
    catch (...)
    {
        error = std::current_exception();
    }
    if (_rendererInitialized)
    {
        _rendererInitialized = false;
        try
        {
            _renderer->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    }
    _renderer.reset();
    _initialConfiguration = {};
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void DefaultRenderLoopDelegate::initializeRenderer()
{
    if (!_renderer || _rendererInitialized || !_sceneReady || _outputExtent.isZero())
    {
        return;
    }
    auto configuration = std::move(_initialConfiguration);
    configuration.outputExtent = _outputExtent;
    configuration.outputFormat = _outputFormat;
    if (_followOutputExtent)
    {
        configuration.internalExtent = _outputExtent;
    }
    if (configuration.resolution)
    {
        configuration.internalExtent = configuration.resolution->resolve(_outputExtent, configuration.internalExtent);
    }
    for (auto& layer : configuration.layers)
    {
        layer.configuration.outputExtent = configuration.outputExtent;
        layer.configuration.internalExtent = configuration.internalExtent;
    }
    Renderer::validateConfiguration(configuration);
    _rendererInitialized = true; // Cleanup must cover partial initialization too.
    try
    {
        _renderer->initialize(*_engine, std::move(configuration));
        _renderer->attachScene(_scene);
        attachTargets();
        // Attachment callbacks may restore resolution/effect quality. Rebuild
        // before the first record, including surfaceChanged after acquisition.
        reconfigureRenderer();
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            releaseRenderer();
        }
        catch (...)
        {
        }
        std::rethrow_exception(error);
    }
}

void DefaultRenderLoopDelegate::enqueueRendererUpdate(uint64_t generation,
    std::function<void(Renderer&)> operation, std::function<void()> stale)
{
    if (!operation)
    {
        throw std::invalid_argument("Renderer update requires an operation");
    }
    _updates.push_back({generation, std::move(operation), std::move(stale)});
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::applyUpdates(std::vector<PendingUpdate> updates)
{
    if (updates.empty())
    {
        return;
    }
    base::CpuProfileScope::count("delegate.queuedRendererUpdateBatch", updates.size());
    {
        base::CpuProfileScope profile("delegate.waitIdle.queuedRendererUpdates");
        base::CpuProfileScope::count("delegate.queuedRendererUpdateDrains", 1);
        _engine->device()->waitIdle();
    }
    base::CpuProfileScope updateProfile("delegate.applyQueuedRendererUpdates");
    for (auto& update : updates)
    {
        if (!withRenderer(update.generation, update.operation))
        {
            if (update.stale)
            {
                update.stale();
            }
        }
        else
        {
            requestSceneFrame();
        }
    }
}

void DefaultRenderLoopDelegate::prepareFrame()
{
    if (!_engine || !_sceneReady)
    {
        throw std::logic_error("Default draw delegate scene is not initialized");
    }
    base::CpuProfileScope prepareProfile("delegate.prepareFrame");
    base::CpuProfileScope::count("delegate.prepareFrameCalls", 1);
    auto updates = std::move(_updates);
    _updates.clear(); // Updates queued by callbacks wait for the next prepare hook.
    base::CpuProfileScope::count("delegate.pendingRendererUpdates", updates.size());
    base::CpuProfileScope::count("delegate.rendererReplacementPending", _pendingRenderer ? 1 : 0);
    base::CpuProfileScope::count("delegate.sceneReplacementPending", _scenePending ? 1 : 0);
    if (!_pendingRenderer && !_scenePending)
    {
        base::CpuProfileScope stageProfile("delegate.applyQueuedUpdatesAndReconfigure");
        applyUpdates(std::move(updates));
        reconfigureRenderer();
        stageProfile.next("delegate.rendererPrepareFrame");
        if (_rendererInitialized && _renderer->prepareFrame())
        {
            requestSceneFrame();
        }
        return;
    }
    // This hook runs before acquisition or UI callbacks. Background producers
    // remain the application's responsibility and must not submit concurrently.
    {
        base::CpuProfileScope profile(_pendingRenderer
            ? (_scenePending ? "delegate.waitIdle.rendererAndSceneReplacement" : "delegate.waitIdle.rendererReplacement")
            : "delegate.waitIdle.sceneReplacement");
        base::CpuProfileScope::count("delegate.replacementDrains", 1);
        _engine->device()->waitIdle();
    }
    base::CpuProfileScope stageProfile("delegate.commitReplacement");
    // Snapshot before callbacks: replacements queued by detach/attach run later.
    auto pending = std::move(_pendingRenderer);
    _pendingRenderer.reset();
    const bool scenePending = _scenePending;
    auto sceneRoot = scenePending ? std::move(_pendingScene) : _scene;
    _scenePending = false;
    if (pending)
    {
        stageProfile.next("delegate.releaseRenderer");
        releaseRenderer();
        stageProfile.next("delegate.installRendererReplacement");
        _renderer = std::move(pending->renderer);
        _initialConfiguration = std::move(pending->configuration);
        _followOutputExtent = _initialConfiguration.internalExtent.isZero();
        _fixedInternalExtent = _initialConfiguration.internalExtent;
    }
    else
    {
        stageProfile.next("delegate.detachSceneTargets");
        detachTargets();
    }
    _scene = std::move(sceneRoot);
    if (_rendererInitialized)
    {
        stageProfile.next("delegate.attachScene");
        _renderer->attachScene(_scene);
        stageProfile.next("delegate.attachTargets");
        attachTargets();
    }
    else
    {
        stageProfile.next("delegate.initializeRenderer");
        initializeRenderer();
    }
    stageProfile.next("delegate.applyQueuedUpdates");
    applyUpdates(std::move(updates));
    reconfigureRenderer();
    stageProfile.next("delegate.sceneVisitorUpdate");
    // The new scene/resources are attached at this safe boundary. Give its
    // update visitor one pass before asking the renderer to synchronize
    // scene-derived resources such as its environment map.
    if (_scene)
    {
        scene::UpdateVisitor update;
        update.update(_scene.get(), 0.0f);
    }
    stageProfile.next("delegate.rendererPrepareFrame");
    if (_rendererInitialized && _renderer->prepareFrame())
    {
        requestSceneFrame();
    }
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::setInternalExtent(gpu::Size2D extent)
{
    if ((extent.width == 0) != (extent.height == 0))
    {
        throw std::invalid_argument("Internal extent must have both dimensions or follow output");
    }
    _followOutputExtent = extent.isZero();
    _fixedInternalExtent = extent;
    _initialConfiguration.internalExtent = extent;
    reconfigureRenderer();
}

void DefaultRenderLoopDelegate::reconfigureRenderer()
{
    if (!_rendererInitialized)
    {
        return;
    }
    const auto internal = _renderer->resolveInternalExtent(_outputExtent,
        _followOutputExtent ? _outputExtent : _fixedInternalExtent);
    if (internal != _renderer->configuration().internalExtent || _renderer->needsReconfiguration())
    {
        base::CpuProfileScope reconfigureProfile("delegate.reconfigureRenderer");
        {
            base::CpuProfileScope profile("delegate.waitIdle.rendererReconfiguration");
            base::CpuProfileScope::count("delegate.reconfigurationDrains", 1);
            _engine->device()->waitIdle();
        }
        detachTargets();
        _renderer->resize(_outputExtent, internal);
        attachTargets();
        requestSceneFrame();
    }
}

void DefaultRenderLoopDelegate::surfaceChanged(gpu::Size2D extent, gpu::PixelFormat format)
{
    if (!_engine || !_sceneReady || extent.isZero() || format == gpu::PixelFormat::Undefined || gpu::isDepthFormat(format))
    {
        throw std::invalid_argument("Default draw delegate requires a valid resolved color surface");
    }
    const bool formatChanged = _outputFormat != format;
    _outputExtent = extent;
    _outputFormat = format;
    if (_rendererInitialized)
    {
        // RenderLoop drained all users before notifying this callback.
        detachTargets();
        if (formatChanged)
        {
            _initialConfiguration = _renderer->configuration();
            _initialConfiguration.internalExtent = _followOutputExtent ? gpu::Size2D{} : _fixedInternalExtent;
            _rendererInitialized = false;
            _renderer->cleanup();
            initializeRenderer();
            requestSceneFrame();
            return;
        }
        _renderer->resize(extent, _renderer->resolveInternalExtent(extent,
            _followOutputExtent ? extent : _fixedInternalExtent));
        attachTargets();
    }
    else
    {
        initializeRenderer();
    }
    requestSceneFrame();
}

void DefaultRenderLoopDelegate::resize(gpu::Size2D extent)
{
    if (!_engine)
    {
        throw std::logic_error("Default draw delegate is not initialized");
    }
    surfaceChanged(extent, _engine->renderSurface()->colorFormat());
}

void DefaultRenderLoopDelegate::update(const FrameContext& frame)
{
    if (!_rendererInitialized || !_targetsBound)
    {
        throw std::logic_error("Default draw renderer is not ready");
    }
    _renderer->update(frame);
    if (_continuousRendering)
    {
        requestSceneFrame();
    }
}

void DefaultRenderLoopDelegate::render(const FrameContext& frame)
{
    if (!_rendererInitialized || !_targetsBound)
    {
        throw std::logic_error("Default draw renderer is not ready");
    }
    _renderer->record(frame);
}

void DefaultRenderLoopDelegate::cleanup()
{
    std::exception_ptr error;
    // RenderLoop has already drained GPU users. Direct callers must do so too.
    try
    {
        releaseRenderer();
    }
    catch (...)
    {
        error = std::current_exception();
    }
    _updates.clear();
    _pendingRenderer.reset();
    _pendingScene.reset();
    _scene.reset();
    _scenePending = false;
    _sceneReady = false;
    _outputExtent = {};
    _outputFormat = gpu::PixelFormat::Undefined;
    _detach = nullptr;
    _attach = nullptr;
    _engine = nullptr;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
