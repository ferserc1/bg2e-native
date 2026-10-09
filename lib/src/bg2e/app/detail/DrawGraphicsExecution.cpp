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

#include "GraphicsExecution.hpp"

#include <bg2e/app/Application.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/RenderLoop.hpp>
#include <bg2e/gpu/Factory.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <SDL2/SDL_vulkan.h>
#ifdef BG2E_IS_MAC
#include <SDL2/SDL_metal.h>
#endif

#include <exception>

#include <stdexcept>

namespace bg2e {
namespace app {
namespace detail {

class DrawGraphicsExecution : public GraphicsExecution {
public:
    explicit DrawGraphicsExecution(const draw::EngineConfig& config)
        : _config { config }
    {
    }

    ~DrawGraphicsExecution() override
    {
        try
        {
            cleanup();
        }
        catch (...)
        {
        }
    }

    gpu::WindowType windowType() const override
    {
        if (!_backend) throw std::logic_error("Draw backend has not been prepared.");
        return _backend->windowType();
    }

    void validate(const Application& application) const override
    {
        auto& app = const_cast<Application&>(application);
        if (app.renderDelegate())
        {
            throw std::invalid_argument("MainLoop selected experimental draw, but Application contains a render::RenderLoopDelegate");
        }
        if (!app.drawDelegate())
        {
            throw std::invalid_argument("MainLoop selected experimental draw, but Application does not contain a draw::RenderLoopDelegate");
        }
        if (!app.uiDelegate())
        {
            throw std::invalid_argument("MainLoop selected experimental draw, but Application does not contain a ui::UserInterfaceDelegate");
        }
        if (!app.inputDelegate())
        {
            throw std::invalid_argument("MainLoop selected experimental draw, but Application does not contain an app::InputDelegate");
        }

#ifndef BG2E_IS_MAC
        if (_config.backend == gpu::BackendType::Metal)
        {
            throw std::invalid_argument("MainLoop selected experimental draw with the Metal backend, but Metal is only supported on macOS");
        }
#endif
    }

    void ensureRuntimeAvailable() const override {}
    bool userInterfaceReady() const override { return _uiInitialized; }

    void prepare(const std::string& applicationId) override
    {
        if (_backend) return;
        if (_config.applicationName.empty()) _config.applicationName = applicationId;
        _backend = gpu::Factory::acquireBackend(_config.backend);
    }

    void initialize(
        SDL_Window* window,
        Application& application,
        ui::UserInterface& userInterface
    ) override
    {
        if (!_backend) throw std::logic_error("Draw backend has not been prepared.");
        _engine.init(window, *_backend, _config);
        _engineInitialized = true;
        _userInterface = &userInterface;
        _renderLoop.setDelegate(application.drawDelegate());
        _renderLoop.init(&_engine);
        _lastDrawableSize = drawableSize();
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        application.uiDelegate()->setInitialSize(uint32_t(width), uint32_t(height));
        userInterface.setDelegate(application.uiDelegate());
        userInterface.init(&_engine);
        _uiInitialized = true;
        _renderLoop.setUIFramePreparationCallback([this](auto& command, auto& frame) {
            _userInterface->newFrame(command, frame);
        });
        _renderLoop.setUICompositionCallback([this](auto& command, auto& frame) {
            _userInterface->draw(command, frame);
        });
    }

    void initializeScene() override
    {
        _renderLoop.initScene();
    }

    void requestResize() override
    {
        _resizePending = true;
        _renderLoop.requestResize();
    }

    void frame(float deltaMilliseconds, bool renderingAllowed) override
    {
        if (!renderingAllowed || !_engineInitialized) return;
        const auto size = drawableSize();
        if (size.isZero()) return;
        if (_resizePending || size != _lastDrawableSize)
        {
            _engine.surface()->resize(size);
            _renderLoop.requestResize();
            _lastDrawableSize = size;
            _resizePending = false;
        }
        _renderLoop.frame(deltaMilliseconds / 1000.0f);
    }

    void waitIdle() override
    {
        if (_engineInitialized) _engine.device()->waitIdle();
    }

    void requestSceneFrame() override
    {
        _renderLoop.requestSceneFrame();
    }

    void pauseScene(const glm::vec4& clearColor) override
    {
        _renderLoop.pauseScene(clearColor);
    }

    void resumeScene() override
    {
        _renderLoop.resumeScene();
    }

    void cleanup() override
    {
        // MainLoop has stopped frame production; background producers must also
        // be stopped by their owner before GPU resources are destroyed.
        std::exception_ptr error;
        try { _renderLoop.cleanup(); } catch (...) { error = std::current_exception(); }
        if (_uiInitialized)
        {
            try { _userInterface->cleanup(); } catch (...) { if (!error) error = std::current_exception(); }
            _uiInitialized = false;
        }
        try { _engine.cleanup(); } catch (...) { if (!error) error = std::current_exception(); }
        _engineInitialized = false;
        _userInterface = nullptr;
        _backend.reset();
        if (error) std::rethrow_exception(error);
    }

protected:
    gpu::Size2D drawableSize() const
    {
        int width = 0, height = 0;
        auto* window = _engine.instance()->window();
#ifdef BG2E_IS_MAC
        if (_config.backend == gpu::BackendType::Metal)
            SDL_Metal_GetDrawableSize(window, &width, &height);
        else
#endif
            SDL_Vulkan_GetDrawableSize(window, &width, &height);
        return { uint32_t(width > 0 ? width : 0), uint32_t(height > 0 ? height : 0) };
    }

    gpu::Size2D _lastDrawableSize;
    bool _resizePending = false;
    draw::EngineConfig _config;
    std::shared_ptr<gpu::Backend> _backend; // Outlives Engine and RenderLoop.
    bool _engineInitialized = false;
    bool _uiInitialized = false;
    draw::Engine _engine;
    draw::RenderLoop _renderLoop;
    ui::UserInterface* _userInterface = nullptr;
};

std::unique_ptr<GraphicsExecution> createDrawGraphicsExecution(const draw::EngineConfig& config)
{
    return std::make_unique<DrawGraphicsExecution>(config);
}

}
}
}
