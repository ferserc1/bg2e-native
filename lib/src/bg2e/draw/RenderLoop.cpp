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

#include <bg2e/draw/RenderLoop.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <bg2e/gpu/SubmissionTiming.hpp>
#include <bg2e/gpu/CleanupManager.hpp>

#include <exception>
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cmath>
#include <memory>
#include <sstream>

namespace bg2e::draw {

struct RenderLoop::Impl {
    struct Slot {
        std::shared_ptr<gpu::CommandBuffer> command;
        std::shared_ptr<gpu::SurfaceFrame> frame;
    };
    bool gpuTimingPending = false;

    void collectTimings(const gpu::Device& device) noexcept
    {
        if (!gpuTimingPending && !base::CpuProfiler::enabled())
        {
            return;
        }
        base::CpuProfileScope profile("profiling.gpu.collect");
        const std::array<const gpu::Queue*, 3> queues{
            &device.graphicsQueue(), &device.presentQueue(), &device.transferQueue()};
        std::array<base::GpuProfileQueueStatus, 3> statuses{};
        size_t statusCount = 0;
        gpuTimingPending = false;
        for (size_t i = 0; i < queues.size(); ++i)
        {
            bool duplicate = false;
            for (size_t j = 0; j < i; ++j)
            {
                duplicate |= queues[i] == queues[j];
            }
            if (duplicate)
            {
                continue;
            }
            const auto capabilities = queues[i]->submissionTimingCapabilities();
            const auto batch = queues[i]->pollSubmissionTimings();
            gpuTimingPending |= batch.pending != 0;
            for (const auto& result : batch.results)
            {
                base::CpuProfiler::completeGpu(result);
            }
            size_t index = 0;
            while (index < statusCount && (statuses[index].device != capabilities.device ||
                statuses[index].queue != capabilities.queue))
            {
                ++index;
            }
            if (index == statusCount)
            {
                ++statusCount;
                statuses[index].device = capabilities.device;
                statuses[index].queue = capabilities.queue;
                statuses[index].backend = capabilities.backend;
            }
            // Aliases of one native queue merge bookkeeping counts only, never
            // GPU execution durations or an alleged frame total.
            auto& status = statuses[index];
            status.supported |= capabilities.supported;
            status.pending += batch.pending;
            status.dropped += batch.dropped;
            status.failed += batch.failed;
            status.unsupported += batch.unsupported;
        }
        for (size_t i = 0; i < statusCount; ++i)
        {
            base::CpuProfiler::gpuQueueStatus(statuses[i]);
        }
    }

    std::vector<Slot> slots;
    std::shared_ptr<gpu::Image> sceneColor;
    uint64_t surfaceGeneration = 0;
    bool colorInitialized = false;
};

RenderLoop::RenderLoop() : _impl(std::make_unique<Impl>())
{
}

RenderLoop::~RenderLoop()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
    }
}

void RenderLoop::init(Engine* engine)
{
    if (_engine)
    {
        throw std::logic_error("draw::RenderLoop is already initialized.");
    }
    if (!engine || !_delegate)
    {
        throw std::invalid_argument("draw::RenderLoop requires an Engine and a delegate.");
    }
    if (_delegate->_requestSceneFrame)
    {
        throw std::logic_error("A draw delegate can belong to only one initialized RenderLoop");
    }
    engine->device(); // Require a successfully initialized context.
    if (!engine->surface())
    {
        throw std::invalid_argument("draw::RenderLoop requires a windowed Engine; use OffscreenApplication for headless work.");
    }
    _engine = engine;
    base::CpuProfiler::setContextProvider([](void* contextData)
    {
        auto& loop = *static_cast<RenderLoop*>(contextData);
        std::ostringstream text;
        text << "backend=" << (loop._engine->backendType() == gpu::BackendType::Metal ? "Metal" : "Vulkan")
             << " surface=" << loop._engine->renderSurface()->size().width << 'x'
             << loop._engine->renderSurface()->size().height
             << " presentations=" << loop._presentationCount << " scene_frames=" << loop._renderedSceneCount;
        if (const auto delegate = std::dynamic_pointer_cast<DefaultRenderLoopDelegate>(loop._delegate))
        {
            if (const auto deferred = dynamic_cast<const RendererDeferred*>(delegate->renderer()))
            {
                const auto& config = deferred->configuration();
                text << " renderer=deferred internal=" << config.internalExtent.width << 'x'
                     << config.internalExtent.height
                     << " rt_shadows=" << (deferred->usesQueryShadows() && deferred->softShadows()->settings().enabled ? "on" : "off")
                     << " rt_ao=" << (deferred->ambientOcclusionEnabled() ? "on" : "off")
                     << " rt_gi=" << (deferred->globalIlluminationEnabled() ? "on" : "off")
                     << " rt_reflections=" << (deferred->reflectionsEnabled() ? "on" : "off");
            }
            else
            {
                text << " renderer=custom";
            }
        }
        else
        {
            text << " renderer=custom";
        }
        return text.str();
    }, this);
    _delegate->_requestSceneFrame = [this]()
    {
        requestSceneFrame();
    };
    _delegateInitialized = true; // Clean partially initialized delegates too.
    _delegate->init(engine);
}

void RenderLoop::initScene()
{
    if (!_engine)
    {
        throw std::logic_error("draw::RenderLoop is not initialized.");
    }
    if (_sceneInitialized)
    {
        throw std::logic_error("draw scene is already initialized.");
    }
    _delegate->initScene();
    _sceneInitialized = true;
    requestSceneFrame();
}

void RenderLoop::frame(float deltaSeconds, ui::UserInterface&)
{
    frame(deltaSeconds);
}

void RenderLoop::frame(float deltaSeconds)
{
    base::CpuProfileScope frameProfile("frame.total", true);
    base::CpuProfileScope stageProfile("frame.prepareDelegate");
    if (!_engine || !_sceneInitialized)
    {
        throw std::logic_error("draw scene is not initialized.");
    }
    if (_frameActive)
    {
        throw std::logic_error("draw::RenderLoop frame execution cannot be reentered");
    }
    struct FrameGuard {
        bool& active;
        explicit FrameGuard(bool& value) : active(value)
        {
            active = true;
        }
        ~FrameGuard()
        {
            active = false;
        }
    } guard(_frameActive);
    _impl->collectTimings(*_engine->device());
    _delegate->prepareFrame();
    if (!_scenePaused)
    {
        _sceneDeltaSeconds += deltaSeconds;
        _sceneFrameElapsed += deltaSeconds;
    }
    auto* surface = _engine->surface();
    if (surface->size().isZero())
    {
        return;
    }
    stageProfile.next("frame.acquireAndWaitSlot");
    auto frame = surface->beginFrame();
    stageProfile.next("frame.resizeAndTargets");
    if (!frame)
    {
        return; // No drawable: neither slot nor frame number advances.
    }
    if (!frame->isValid() || !frame->colorImage())
    {
        throw std::runtime_error("Draw acquired an invalid presentation frame.");
    }
    auto* presentationColor = frame->colorImage();
    const auto extent = presentationColor->size();
    const auto format = presentationColor->pixelFormat();
    if (extent.isZero())
    {
        throw std::runtime_error("Draw acquired a zero-sized presentation image.");
    }

    if (!_impl->sceneColor || _resizeRequested ||
        _impl->surfaceGeneration != surface->generation() ||
        _impl->sceneColor->size() != extent || _impl->sceneColor->pixelFormat() != format)
    {
        // A scene target is shared by all slots. Resize drains every user,
        // unlike ordinary frame execution, which waits only during slot reuse.
        {
            base::CpuProfileScope profile("renderLoop.waitIdle.rebuildSceneTarget");
            base::CpuProfileScope::count("renderLoop.sceneTargetRebuildDrains", 1, "drains");
            _engine->device()->waitIdle();
        }
        _impl->slots.clear();
        if (_impl->sceneColor)
        {
            _impl->sceneColor->cleanup();
        }
        _impl->sceneColor.reset();
        gpu::ImageDescription description;
        description.size = extent;
        description.format = format;
        description.usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::Sampled;
        description.debugName = "draw retained scene color";
        _impl->sceneColor = _engine->device()->createImage(description);
        if (!_impl->sceneColor || !_impl->sceneColor->isValid())
        {
            throw std::runtime_error("Failed to create retained draw scene color.");
        }
        _impl->colorInitialized = false;
        _impl->surfaceGeneration = surface->generation();
        _impl->slots.resize(surface->inFlightFrames());
        _resizeRequested = false;
        requestSceneFrame();
        _delegate->surfaceChanged(extent, format);
    }

    const auto slotIndex = surface->currentFrameIndex();
    if (slotIndex >= _impl->slots.size())
    {
        throw std::runtime_error("Invalid draw frame slot.");
    }
    auto& slot = _impl->slots[slotIndex];
    // beginFrame already waited for this slot. New wrappers work on both
    // reusable Vulkan allocations and one-shot Metal command buffers.
    stageProfile.next("frame.retireSlotResources");
    slot.command.reset();
    slot.frame.reset();
    const auto& queue = _engine->device()->graphicsQueue();
    stageProfile.next("frame.createAndBeginCommand");
    slot.command = queue.createCommandBuffer("draw scene and presentation");
    slot.frame = frame;
    auto& command = *slot.command;
    command.begin();
    auto gpuRequest = base::CpuProfiler::beginGpu(_presentationCount + 1, 0,
        base::GpuProfileWorkload::Unassociated);
    if (gpuRequest.token)
    {
        base::CpuProfileScope timingProfile("profiling.gpu.request");
        const auto capabilities = queue.submissionTimingCapabilities();
        gpuRequest.device = capabilities.device;
        gpuRequest.queue = capabilities.queue;
        gpuRequest.backend = capabilities.backend;
        const auto status = command.requestSubmissionTiming(gpuRequest);
        if (status == gpu::SubmissionTimingStatus::Pending)
        {
            _impl->gpuTimingPending = true;
        }
        else
        {
            gpuRequest.status = status;
            base::CpuProfiler::completeGpu(gpuRequest);
        }
    }
    if (_uiPreparation)
    {
        base::CpuProfileScope profile("frame.uiPreparation");
        _uiPreparation(command, *frame);
    }
    if (command.hasActiveScope())
    {
        throw std::logic_error("UI preparation left an active command scope.");
    }
    const bool sceneDue = !_impl->colorInitialized || _sceneMaxFrameRate == 0.0 ||
        _sceneFrameElapsed >= 1.0 / _sceneMaxFrameRate;
    const bool refresh = !_scenePaused && _sceneDirty && sceneDue;
    command.setSubmissionTimingContext(refresh ? _renderedSceneCount + 1 : 0,
        refresh ? (_uiComposition ? gpu::SubmissionTimingWorkload::SceneAndUi : gpu::SubmissionTimingWorkload::Scene) :
            gpu::SubmissionTimingWorkload::Ui);
    const auto revision = _sceneRevision;
    FrameContext context{*_engine, command, *_impl->sceneColor, extent,
                         surface->frameCounter(), slotIndex, deltaSeconds, _renderedSceneCount, _sceneDeltaSeconds};
    if (refresh)
    {
        base::CpuProfileScope profile("frame.delegateUpdate");
        _delegate->update(context);
    }
    if (command.hasActiveScope())
    {
        throw std::logic_error("Draw delegate update left an active command scope.");
    }

    stageProfile.next("frame.recordScene");
    if (!_impl->colorInitialized || refresh)
    {
        command.transition(_impl->sceneColor.get(), gpu::ImageLayout::ColorAttachment);
        command.beginRendering(_impl->sceneColor.get());
        command.clearColor(0, {_sceneClearColor.r, _sceneClearColor.g, _sceneClearColor.b, _sceneClearColor.a});
        command.endRendering();
    }
    if (refresh)
    {
        // Order the clear pass's writes before a delegate pass loads the color.
        command.transition(_impl->sceneColor.get(), gpu::ImageLayout::ColorAttachment);
        // The delegate owns any rendering/compute scope it opens, with color
        // already in ColorAttachment layout. This initial contract has no depth.
        _delegate->render(context);
        if (command.hasActiveScope())
        {
            throw std::logic_error("Draw delegate render left an active command scope.");
        }
    }
    stageProfile.next("frame.copyAndUI");
    command.transition(_impl->sceneColor.get(), gpu::ImageLayout::TransferSrc);
    command.transition(presentationColor, gpu::ImageLayout::TransferDst);
    command.copyImage(_impl->sceneColor.get(), presentationColor);
    command.transition(presentationColor, gpu::ImageLayout::ColorAttachment);
    if (_uiComposition)
    {
        base::CpuProfileScope profile("frame.uiComposition");
        gpu::GpuTimingScope gpuProfile(command, "frame.uiComposition");
        _uiComposition(command, *frame);
    }
    if (command.hasActiveScope())
    {
        throw std::logic_error("Draw UI composition left an active command scope.");
    }
    command.transition(presentationColor, gpu::ImageLayout::Present);
    stageProfile.next("frame.encodePresentAndEnd");
    surface->present(&command);
    command.end();
    stageProfile.next("frame.submit");
    queue.submit(&command);
    stageProfile.next("frame.endFrame");
    _impl->colorInitialized = true;
    if (refresh)
    {
        ++_renderedSceneCount;
        _sceneDeltaSeconds = 0.0f;
        _sceneFrameElapsed = _sceneMaxFrameRate > 0.0
            ? std::fmod(_sceneFrameElapsed, 1.0 / _sceneMaxFrameRate) : 0.0;
        if (revision == _sceneRevision)
        {
            _sceneDirty = false;
        }
    }
    surface->endFrame(frame.get());
    ++_presentationCount;
    frameProfile.frameCompleted(_presentationCount, _renderedSceneCount, refresh);
    stageProfile.next("frame.deferredCleanup");
    _engine->cleanupManager().flushDeferred();
    _impl->collectTimings(*_engine->device());
}

void RenderLoop::setUICompositionCallback(UICompositionCallback callback)
{
    _uiComposition = std::move(callback);
}

void RenderLoop::setUIFramePreparationCallback(UICompositionCallback callback)
{
    _uiPreparation = std::move(callback);
}

void RenderLoop::requestResize()
{
    _resizeRequested = true;
    requestSceneFrame();
}

void RenderLoop::setSceneMaxFrameRate(double fps)
{
    if (!std::isfinite(fps) || fps < 0.0)
    {
        throw std::invalid_argument("Scene frame rate must be finite and nonnegative");
    }
    if (_sceneMaxFrameRate != fps)
    {
        _sceneMaxFrameRate = fps;
        _sceneFrameElapsed = fps > 0.0 ? 1.0 / fps : 0.0;
    }
}

void RenderLoop::setSceneClearColor(const glm::vec4& clearColor)
{
    if (_sceneClearColor != clearColor)
    {
        _sceneClearColor = clearColor;
        requestSceneFrame();
    }
}

void RenderLoop::pauseScene(const glm::vec4& clearColor)
{
    _scenePaused = true;
    _sceneDeltaSeconds = 0.0f;
    setSceneClearColor(clearColor); // Preserve the last valid image while paused.
}

void RenderLoop::resumeScene()
{
    _scenePaused = false;
    requestSceneFrame();
}

void RenderLoop::requestSceneFrame()
{
    _sceneDirty = true;
    ++_sceneRevision;
}

void RenderLoop::cleanup()
{
    if (_frameActive)
    {
        throw std::logic_error("Cannot clean up draw execution during a frame callback");
    }
    if (!_engine)
    {
        return;
    }
    // Frame/background producers must remain stopped throughout shutdown.
    std::exception_ptr error;
    try
    {
        {
            base::CpuProfileScope profile("renderLoop.waitIdle.cleanup");
            base::CpuProfileScope::count("renderLoop.cleanupDrains", 1, "drains");
            _engine->device()->waitIdle();
        }
        _impl->collectTimings(*_engine->device());
    }
    catch (...)
    {
        error = std::current_exception();
    }
    // Disconnect the borrowed loop callback even if delegate cleanup throws.
    _delegate->_requestSceneFrame = nullptr;
    base::CpuProfiler::setContextProvider(nullptr, nullptr);
    if (_delegateInitialized)
    {
        _delegateInitialized = false;
        try
        {
            _delegate->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    }
    _uiComposition = nullptr;
    _uiPreparation = nullptr;
    _impl->slots.clear();
    if (_impl->sceneColor)
    {
        try
        {
            _impl->sceneColor->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
        _impl->sceneColor.reset();
    }
    _impl->colorInitialized = false;
    _impl->surfaceGeneration = 0;
    _engine = nullptr;
    _sceneInitialized = false;
    _resizeRequested = false;
    _scenePaused = false;
    _sceneDirty = true;
    _sceneRevision = 0;
    _presentationCount = 0;
    _renderedSceneCount = 0;
    _sceneDeltaSeconds = 0.0f;
    _sceneClearColor = {0.f, 0.f, 0.f, 1.f};
    _sceneFrameElapsed = 0.0;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
