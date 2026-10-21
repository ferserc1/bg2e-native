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
        cleanup();
    }
    catch (...)
    {
        // Explicit run/cleanup reports errors; destruction cannot throw.
    }
}

void OffscreenApplication::init(
    int argc, char** argv,
    const std::string& appId,
    std::shared_ptr<DrawOffscreenApplicationDelegate> delegate)
{
    if (_initialized || _initializing || _running || _cleaning)
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
        _delegate = std::move(delegate);
        _config = std::move(config);
        _appId = appId;
        auto backend = gpu::Factory::acquireBackend(_config.engine.backend);
        _engine = std::make_unique<draw::Engine>();
        _engine->init(_config.extent, *backend, _config.engine);
        _engineReady = true;
        _delegateInitialized = true; // Covers partial initialization callbacks.
        _delegate->init(_engine.get(), _engine->offscreenSurface());
        _delegate->initScene();
        _delegate->resize(_config.extent);
        _delegate->prepareFrame();
        _initialized = true;
        _initializing = false;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            cleanup();
        }
        catch (...)
        {
        }
        _initializing = false;
        std::rethrow_exception(error);
    }
}

int OffscreenApplication::run()
{
    if (!_initialized || _running || _cleaning || _initializing || !_engineReady || !_delegateInitialized)
    {
        throw std::logic_error("Offscreen draw execution is not ready or is already running");
    }
    _running = true;
    try
    {
        auto* device = _engine->device();
        auto* surface = _engine->offscreenSurface();
        uint64_t completed = 0;
        while (_delegate->continueRendering())
        {
            if (completed == std::numeric_limits<uint64_t>::max())
            {
                throw std::overflow_error("Offscreen scene frame sequence exhausted");
            }
            _delegate->prepareFrame();
            const float delta = completed == 0 ? 0.0f : _config.timeStepSeconds;
            _delegate->update(delta, completed);
            if (!_delegate->continueRendering())
            {
                break;
            }
            auto frame = surface->beginFrame(); // Waits for completed-slot reuse.
            if (!frame || !frame->isValid() || !frame->colorImage() ||
                frame->colorImage()->size() != _config.extent ||
                frame->colorImage()->pixelFormat() != _config.engine.colorFormat)
            {
                throw std::runtime_error("Offscreen draw acquired an invalid frame");
            }
            const auto slot = surface->currentFrameIndex();
            if (slot >= _engine->frameSlotCount())
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
                draw::FrameContext context{*_engine, *command, *color, color->size(),
                    completed, slot, delta, completed, delta};
                _delegate->render(context);
                if (command->hasActiveScope())
                {
                    throw std::logic_error("Offscreen draw delegate left an active command scope");
                }
                command->transition(color, gpu::ImageLayout::TransferSrc);
                // Offscreen present associates completion; it never presents to a window.
                surface->present(command);
            });
            // immediateSubmit returns only after this scene submission completes.
            _delegate->completeFrame(completed);
            if (_delegate->needsResolve())
            {
                device->immediateSubmit([&](gpu::CommandBuffer* command)
                {
                    auto* color = frame->colorImage();
                    draw::FrameContext context{*_engine, *command, *color, color->size(),
                        completed, slot, delta, completed, delta};
                    _delegate->resolveFrame(context);
                    if (command->hasActiveScope())
                    {
                        throw std::logic_error("Offscreen final conversion left an active command scope");
                    }
                    command->transition(color, gpu::ImageLayout::TransferSrc);
                    surface->present(command);
                });
                _delegate->completeResolve();
            }
            surface->endFrame(frame.get());
            const double elapsedMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            const auto frameNumber = completed++;
            _engine->cleanupManager().flushDeferred();
            _delegate->didRenderFrame(frameNumber, elapsedMs);
        }
        _delegate->finish();
        cleanup();
        _running = false;
        return 0;
    }
    catch (...)
    {
        const auto error = std::current_exception();
        try
        {
            cleanup();
        }
        catch (...)
        {
        }
        _running = false;
        std::rethrow_exception(error);
    }
}

void OffscreenApplication::cleanup()
{
    if (_cleaning || !_engine)
    {
        return;
    }
    _cleaning = true;
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
    if (_engineReady)
    {
        attempt([&]
        {
            _engine->device()->waitIdle();
        });
    }
    if (_delegateInitialized)
    {
        _delegateInitialized = false;
        attempt([&]
        {
            _delegate->cleanup();
        });
    }
    _engineReady = false;
    attempt([&]
    {
        _engine->cleanup();
    });
    _engine.reset();
    _cleaning = false;
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
