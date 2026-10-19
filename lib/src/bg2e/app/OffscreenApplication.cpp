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

#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/render/vulkan/all.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Factory.hpp>
#include <bg2e/gpu/Backend.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/OffscreenSurface.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/CleanupManager.hpp>

#include <chrono>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::app {

OffscreenApplication::OffscreenApplication() = default;

OffscreenApplication::~OffscreenApplication()
{
    try
    {
        cleanupDraw();
    }
    catch (...)
    {
        // Explicit run/cleanup reports errors; destruction cannot throw.
    }
}

void OffscreenApplication::init(
    int argc, char** argv,
    const std::string& appId,
    std::shared_ptr<OffscreenApplicationDelegate> delegate)
{
    if (_initialized || _initializing || _drawRunning || _drawCleaning)
    {
        throw std::logic_error("OffscreenApplication::init(): this application is already initialized");
    }
    if (!delegate)
    {
        throw std::invalid_argument("OffscreenApplication::init(): production delegate must not be null");
    }

    _delegate = std::move(delegate);
    _drawDelegate.reset();
    _executionMode = OffscreenExecutionMode::ProductionRender;
    _appId = appId;
    _delegate->initConfig(argc, argv, _config);

    _engine = std::make_unique<render::Engine>();
    _engine->init();

    if (_config.createColorImage || _config.createDepthImage)
    {
        // All the flags needed to use the image for a color buffer or
        // in a compute shader
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_STORAGE_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _colorImage = std::shared_ptr<render::vulkan::Image>(
            render::vulkan::Image::createAllocatedImage(
                _engine.get(),
                "OffscreenApplication color image buffer",
                _config.colorFormat,
                {_config.width, _config.height},
                usage,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 20, VK_SAMPLE_COUNT_1_BIT));
        _engine->cleanupManager().push([this](VkDevice dev)
                                       {
                                           _colorImage->cleanup();
                                       });
    }

    if (_config.createDepthImage)
    {
        _depthImage = std::shared_ptr<render::vulkan::Image>(
            render::vulkan::Image::createAllocatedImage(
                _engine.get(),
                "OffscreenApplication depth image buffer",
                VK_FORMAT_D32_SFLOAT,
                {_config.width, _config.height},
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                1, false, 20, VK_SAMPLE_COUNT_1_BIT));
        render::vulkan::Image::transitionImage(
            _engine.get(),
            _depthImage->handle(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        _engine->cleanupManager().push([this](VkDevice dev)
                                       {
                                           _depthImage->cleanup();
                                       });
    }

    _delegate->init(_engine.get(), _colorImage, _depthImage);
    _engine->iterateFrameResources([&](render::vulkan::FrameResources& frameResources)
                                   {
                                       frameResources.descriptorAllocator->init(_engine.get());
                                       _delegate->initFrameResources(frameResources.descriptorAllocator);
                                   });
    _engine->descriptorSetAllocator().initPool();
    _delegate->initScene();
    _initialized = true;
}

void OffscreenApplication::init(
    int argc, char** argv,
    const std::string& appId,
    std::shared_ptr<DrawOffscreenApplicationDelegate> delegate)
{
    if (_initialized || _initializing || _drawRunning || _drawCleaning)
    {
        throw std::logic_error("OffscreenApplication::init(): this application is already initialized");
    }
    if (!delegate)
    {
        throw std::invalid_argument("OffscreenApplication::init(): draw delegate must not be null");
    }

    _initializing = true;
    try
    {
        DrawOffscreenApplicationConfig config;
        delegate->initConfig(argc, argv, config);
        if (config.frames == 0 || config.frames > 1048576 ||
            (config.internalExtentOverride && config.internalExtentOverride->isZero()) ||
            (config.internalExtentOverride && config.scaleOverride) ||
            (config.scaleOverride && (!std::isfinite(*config.scaleOverride) || *config.scaleOverride < 0.25f || *config.scaleOverride > 1.0f)) ||
            config.extent.isZero() || !std::isfinite(config.timeStepSeconds) || config.timeStepSeconds < 0.0f ||
            !gpu::pixelFormatBytes(config.engine.colorFormat) || gpu::isDepthFormat(config.engine.colorFormat) ||
            gpu::isUnsignedIntegerFormat(config.engine.colorFormat) || gpu::isSignedIntegerFormat(config.engine.colorFormat) ||
            (config.createDepthImage && !gpu::isDepthFormat(config.engine.depthFormat)))
        {
            throw std::invalid_argument("Offscreen draw extent, sample count, overrides, time step or target formats are invalid");
        }
        if (!config.createDepthImage)
        {
            config.engine.depthFormat = gpu::PixelFormat::Undefined;
        }
        if (config.engine.applicationName.empty())
        {
            config.engine.applicationName = appId;
        }
        if (!config.graphicsConfig.empty())
        {
            config.graphicsDocument = draw::SettingsCodec::readFile(config.graphicsConfig);
        }
        delegate->configure(config);
        _drawDelegate = std::move(delegate);
        _delegate.reset();
        _drawConfig = std::move(config);
        _executionMode = OffscreenExecutionMode::ExperimentalDraw;
        _appId = appId;
        auto backend = gpu::Factory::acquireBackend(_drawConfig.engine.backend);
        _drawEngine = std::make_unique<draw::Engine>();
        _drawEngine->init(_drawConfig.extent, *backend, _drawConfig.engine);
        _drawEngineReady = true;
        _drawDelegateInitialized = true; // Covers partial initialization callbacks.
        _drawDelegate->init(_drawEngine.get(), _drawEngine->offscreenSurface());
        _drawDelegate->initScene();
        _drawDelegate->resize(_drawConfig.extent);
        _drawDelegate->prepareFrame();
        _initialized = true;
        _initializing = false;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            cleanupDraw();
        }
        catch (...)
        {
        }
        _initializing = false;
        std::rethrow_exception(error);
    }
}

OffscreenExecutionMode OffscreenApplication::executionMode() const
{
    return _executionMode;
}

int OffscreenApplication::run()
{
    if (!_initialized)
    {
        throw std::runtime_error("OffscreenApplication::run(): the application is not initialized");
    }
    if (_executionMode == OffscreenExecutionMode::ExperimentalDraw)
    {
        return runDraw();
    }
    if (!_engine)
    {
        throw std::runtime_error("OffscreenApplication::run(): the production engine is not initialized");
    }

    VkDevice device = _engine->device().handle();
    auto graphicsQueue = _engine->device().graphicsQueue();
    auto commandPool = _engine->command().createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto commandBuffer = _engine->command().allocateCommandBuffer(commandPool, 1);
    auto frameResources = _engine->currentFrameResources();

    _engine->cleanupManager().push([&](VkDevice dev)
                                   {
                                       vkDestroyCommandPool(dev, commandPool, nullptr);
                                   });
    auto frameFence = frameResources.frameFence;

    _delegate->resize(_config.width, _config.height);

    bool renderFrame = true;
    double elapsedMs = 0.0;
    uint32_t frameIndex = 0;

    VkImageLayout colorImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    while (renderFrame)
    {
        using namespace render::vulkan;

        VK_ASSERT(vkResetFences(device, 1, &frameFence));

        frameResources.flushFrameData();

        _delegate->setDelta(static_cast<float>(elapsedMs));
        _delegate->frame(static_cast<float>(elapsedMs), frameIndex, frameResources);

        auto cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

        if (_colorImage != nullptr && colorImageLayout != VK_IMAGE_LAYOUT_GENERAL)
        {
            render::vulkan::Image::cmdTransitionImage(
                commandBuffer,
                _colorImage->handle(),
                colorImageLayout,
                VK_IMAGE_LAYOUT_GENERAL);
            colorImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        VkImageLayout finalLayout = colorImageLayout;
        renderFrame = _delegate->render(
            commandBuffer,
            frameIndex,
            frameResources,
            colorImageLayout,
            finalLayout);
        colorImageLayout = finalLayout;

        if (_colorImage != nullptr && colorImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            render::vulkan::Image::cmdTransitionImage(
                commandBuffer,
                _colorImage->handle(),
                colorImageLayout,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            colorImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }

        VK_ASSERT(vkEndCommandBuffer(commandBuffer));

        auto cmdInfo = Info::commandBufferSubmitInfo(commandBuffer);
        auto submitInfo = Info::submitInfo(&cmdInfo, nullptr, nullptr);

        auto startTime = std::chrono::high_resolution_clock::now();
        VK_ASSERT(queueSubmit2(graphicsQueue, 1, &submitInfo, frameFence));
        VK_ASSERT(vkWaitForFences(device, 1, &frameFence, true, 100000000000));
        auto endTime = std::chrono::high_resolution_clock::now();

        elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        frameResources.flushFrameData();
        _engine->flushDeferredExec();
        _delegate->didRenderFrame(
            frameIndex,
            elapsedMs,
            colorImageLayout);

        ++frameIndex;
        _engine->nextFrame();
    }

    cleanup();

    return 0;
}

int OffscreenApplication::runDraw()
{
    if (_drawRunning || _drawCleaning || _initializing || !_drawEngineReady || !_drawDelegateInitialized)
    {
        throw std::logic_error("Offscreen draw execution is not ready or is already running");
    }
    _drawRunning = true;
    try
    {
        auto* device = _drawEngine->device();
        auto* surface = _drawEngine->offscreenSurface();
        uint64_t completed = 0;
        while (_drawDelegate->continueRendering())
        {
            if (completed == std::numeric_limits<uint64_t>::max())
            {
                throw std::overflow_error("Offscreen scene frame sequence exhausted");
            }
            _drawDelegate->prepareFrame();
            const float delta = completed == 0 ? 0.0f : _drawConfig.timeStepSeconds;
            _drawDelegate->update(delta, completed);
            if (!_drawDelegate->continueRendering())
            {
                break;
            }
            auto frame = surface->beginFrame(); // Waits for completed-slot reuse.
            if (!frame || !frame->isValid() || !frame->colorImage() ||
                frame->colorImage()->size() != _drawConfig.extent ||
                frame->colorImage()->pixelFormat() != _drawConfig.engine.colorFormat)
            {
                throw std::runtime_error("Offscreen draw acquired an invalid frame");
            }
            const auto slot = surface->currentFrameIndex();
            if (slot >= _drawEngine->frameSlotCount())
            {
                throw std::runtime_error("Offscreen draw frame slot is invalid");
            }
            const auto start = std::chrono::steady_clock::now();
            device->immediateSubmit([&](gpu::CommandBuffer* command)
            {
                auto* color = frame->colorImage();
                command->transition(color, gpu::ImageLayout::ColorAttachment);
                command->beginRendering(color);
                command->clearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
                command->endRendering();
                command->transition(color, gpu::ImageLayout::ColorAttachment);
                draw::FrameContext context{*_drawEngine, *command, *color, color->size(),
                    completed, slot, delta, completed, delta};
                _drawDelegate->render(context);
                if (command->hasActiveScope())
                {
                    throw std::logic_error("Offscreen draw delegate left an active command scope");
                }
                command->transition(color, gpu::ImageLayout::TransferSrc);
                // Offscreen present associates completion; it never presents to a window.
                surface->present(command);
            });
            // immediateSubmit returns only after this scene submission completes.
            _drawDelegate->completeFrame(completed);
            if (_drawDelegate->needsResolve())
            {
                device->immediateSubmit([&](gpu::CommandBuffer* command)
                {
                    auto* color = frame->colorImage();
                    draw::FrameContext context{*_drawEngine, *command, *color, color->size(),
                        completed, slot, delta, completed, delta};
                    _drawDelegate->resolveFrame(context);
                    if (command->hasActiveScope())
                    {
                        throw std::logic_error("Offscreen final conversion left an active command scope");
                    }
                    command->transition(color, gpu::ImageLayout::TransferSrc);
                    surface->present(command);
                });
                _drawDelegate->completeResolve();
            }
            surface->endFrame(frame.get());
            const double elapsedMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            const auto frameNumber = completed++;
            _drawEngine->cleanupManager().flushDeferred();
            _drawDelegate->didRenderFrame(frameNumber, elapsedMs);
        }
        _drawDelegate->finish();
        cleanupDraw();
        _drawRunning = false;
        return 0;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            cleanupDraw();
        }
        catch (...)
        {
        }
        _drawRunning = false;
        std::rethrow_exception(error);
    }
}

void OffscreenApplication::cleanupDraw()
{
    if (_drawCleaning || !_drawEngine)
    {
        return;
    }
    _drawCleaning = true;
    _initialized = false;
    std::exception_ptr error;
    const auto attempt = [&error](auto&& operation)
    {
        try
        {
            operation();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    };
    if (_drawEngineReady)
    {
        attempt([&]
        {
            _drawEngine->device()->waitIdle();
        });
    }
    if (_drawDelegateInitialized)
    {
        _drawDelegateInitialized = false;
        attempt([&]
        {
            _drawDelegate->cleanup();
        });
    }
    _drawEngineReady = false;
    attempt([&]
    {
        _drawEngine->cleanup();
    });
    _drawEngine.reset();
    _drawCleaning = false;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

void OffscreenApplication::cleanup()
{
    if (_executionMode == OffscreenExecutionMode::ExperimentalDraw)
    {
        cleanupDraw();
        return;
    }
    if (_delegate)
    {
        _delegate->cleanup();
    }
    if (_engine)
    {
        _engine->cleanup();
    }
    _initialized = false;
}

}
