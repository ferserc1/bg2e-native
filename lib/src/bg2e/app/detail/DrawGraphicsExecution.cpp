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
        return _config.backend == gpu::BackendType::Metal
            ? gpu::WindowType::Metal
            : gpu::WindowType::Vulkan;
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

    void ensureRuntimeAvailable() const override
    {
        // Intentional milestone 01 boundary: reported before SDL/GPU resources
        // are created. Removed or turned into a no-op in milestone 02.
        throw std::logic_error("Experimental draw execution requires milestone 02");
    }

    void initialize(
        SDL_Window* window,
        Application& application,
        ui::UserInterface& userInterface
    ) override
    {
        _userInterface = &userInterface;
        _renderLoop.setDelegate(application.drawDelegate());
        throw std::logic_error("Experimental draw execution requires milestone 02");
    }

    void initializeScene() override
    {
        _renderLoop.initScene();
    }

    void requestResize() override
    {
        _renderLoop.requestResize();
    }

    void frame(float deltaMilliseconds, bool renderingAllowed) override
    {
        // The outer frame time is milliseconds; the draw API uses seconds.
        if (renderingAllowed && _userInterface)
        {
            _renderLoop.frame(deltaMilliseconds / 1000.0f, *_userInterface);
        }
    }

    void waitIdle() override
    {
        // No GPU resources are allocated in milestone 01.
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
        _renderLoop.cleanup();
        _engine.cleanup();
        _userInterface = nullptr;
    }

protected:
    draw::EngineConfig _config;
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
