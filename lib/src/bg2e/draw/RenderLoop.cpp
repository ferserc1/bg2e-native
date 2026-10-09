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
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/Queue.hpp>
#include <bg2e/gpu/CleanupManager.hpp>

#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bg2e::draw {

struct RenderLoop::Impl {
    struct Slot {
        std::shared_ptr<gpu::CommandBuffer> command;
        std::shared_ptr<gpu::SurfaceFrame> frame;
    };
    std::vector<Slot> slots;
    std::shared_ptr<gpu::Image> sceneColor;
    uint64_t surfaceGeneration = 0;
    bool colorInitialized = false;
};

RenderLoop::RenderLoop() : _impl(std::make_unique<Impl>()) {}
RenderLoop::~RenderLoop()
{
    try { cleanup(); } catch (...) { }
}

void RenderLoop::init(Engine* engine)
{
    if (_engine) throw std::logic_error("draw::RenderLoop is already initialized.");
    if (!engine || !_delegate) throw std::invalid_argument("draw::RenderLoop requires an Engine and a delegate.");
    engine->device(); // Require a successfully initialized context.
    _engine = engine;
    _delegateInitialized = true; // Clean partially initialized delegates too.
    _delegate->init(engine);
}

void RenderLoop::initScene()
{
    if (!_engine) throw std::logic_error("draw::RenderLoop is not initialized.");
    if (_sceneInitialized) throw std::logic_error("draw scene is already initialized.");
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
    if (!_engine || !_sceneInitialized) throw std::logic_error("draw scene is not initialized.");
    auto* surface = _engine->surface();
    if (surface->size().isZero()) return;
    auto frame = surface->beginFrame();
    if (!frame) return; // No drawable: neither slot nor frame number advances.
    if (!frame->isValid() || !frame->colorImage())
        throw std::runtime_error("Draw acquired an invalid presentation frame.");
    auto* presentationColor = frame->colorImage();
    const auto extent = presentationColor->size();
    const auto format = presentationColor->pixelFormat();
    if (extent.isZero()) throw std::runtime_error("Draw acquired a zero-sized presentation image.");

    if (!_impl->sceneColor || _resizeRequested ||
        _impl->surfaceGeneration != surface->generation() ||
        _impl->sceneColor->size() != extent || _impl->sceneColor->pixelFormat() != format)
    {
        // A scene target is shared by all slots. Resize drains every user,
        // unlike ordinary frame execution, which waits only during slot reuse.
        _engine->device()->waitIdle();
        _impl->slots.clear();
        if (_impl->sceneColor) _impl->sceneColor->cleanup();
        _impl->sceneColor.reset();
        gpu::ImageDescription description;
        description.size = extent;
        description.format = format;
        description.usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
        description.debugName = "draw retained scene color";
        _impl->sceneColor = _engine->device()->createImage(description);
        if (!_impl->sceneColor || !_impl->sceneColor->isValid())
            throw std::runtime_error("Failed to create retained draw scene color.");
        _impl->colorInitialized = false;
        _impl->surfaceGeneration = surface->generation();
        _impl->slots.resize(surface->inFlightFrames());
        _resizeRequested = false;
        requestSceneFrame();
        _delegate->resize(extent);
    }

    const auto slotIndex = surface->currentFrameIndex();
    if (slotIndex >= _impl->slots.size()) throw std::runtime_error("Invalid draw frame slot.");
    auto& slot = _impl->slots[slotIndex];
    // beginFrame already waited for this slot. New wrappers work on both
    // reusable Vulkan allocations and one-shot Metal command buffers.
    slot.command.reset();
    slot.frame.reset();
    const auto& queue = _engine->device()->graphicsQueue();
    slot.command = queue.createCommandBuffer("draw scene and presentation");
    slot.frame = frame;
    auto& command = *slot.command;
    command.begin();
    if (_uiPreparation) _uiPreparation(command, *frame);
    if (command.hasActiveScope()) throw std::logic_error("UI preparation left an active command scope.");
    const bool refresh = !_scenePaused && _sceneDirty;
    const auto revision = _sceneRevision;
    FrameContext context{ *_engine, command, *_impl->sceneColor, extent,
        surface->frameCounter(), slotIndex, deltaSeconds };
    if (refresh) _delegate->update(context);
    if (command.hasActiveScope()) throw std::logic_error("Draw delegate update left an active command scope.");

    if (!_impl->colorInitialized || refresh)
    {
        command.transition(_impl->sceneColor.get(), gpu::ImageLayout::ColorAttachment);
        command.beginRendering(_impl->sceneColor.get());
        command.clearColor(0, { _sceneClearColor.r, _sceneClearColor.g, _sceneClearColor.b, _sceneClearColor.a });
        command.endRendering();
    }
    if (refresh)
    {
        // Order the clear pass's writes before a delegate pass loads the color.
        command.transition(_impl->sceneColor.get(), gpu::ImageLayout::ColorAttachment);
        // The delegate owns any rendering/compute scope it opens, with color
        // already in ColorAttachment layout. This initial contract has no depth.
        _delegate->render(context);
        if (command.hasActiveScope()) throw std::logic_error("Draw delegate render left an active command scope.");
    }
    command.transition(_impl->sceneColor.get(), gpu::ImageLayout::TransferSrc);
    command.transition(presentationColor, gpu::ImageLayout::TransferDst);
    command.copyImage(_impl->sceneColor.get(), presentationColor);
    command.transition(presentationColor, gpu::ImageLayout::ColorAttachment);
    if (_uiComposition) _uiComposition(command, *frame);
    if (command.hasActiveScope()) throw std::logic_error("Draw UI composition left an active command scope.");
    command.transition(presentationColor, gpu::ImageLayout::Present);
    surface->present(&command);
    command.end();
    queue.submit(&command);
    _impl->colorInitialized = true;
    if (refresh && revision == _sceneRevision) _sceneDirty = false;
    surface->endFrame(frame.get());
    _engine->cleanupManager().flushDeferred();
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

void RenderLoop::setSceneClearColor(const glm::vec4& clearColor)
{
    if (_sceneClearColor != clearColor) { _sceneClearColor = clearColor; requestSceneFrame(); }
}

void RenderLoop::pauseScene(const glm::vec4& clearColor)
{
    _scenePaused = true;
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
    if (!_engine) return;
    // Frame/background producers must remain stopped throughout shutdown.
    std::exception_ptr error;
    try { _engine->device()->waitIdle(); } catch (...) { error = std::current_exception(); }
    if (_delegateInitialized)
    {
        _delegateInitialized = false;
        try { _delegate->cleanup(); } catch (...) { if (!error) error = std::current_exception(); }
    }
    _uiComposition = nullptr;
    _uiPreparation = nullptr;
    _impl->slots.clear();
    if (_impl->sceneColor)
    {
        try { _impl->sceneColor->cleanup(); } catch (...) { if (!error) error = std::current_exception(); }
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
    _sceneClearColor = { 0.f, 0.f, 0.f, 1.f };
    if (error) std::rethrow_exception(error);
}

}
