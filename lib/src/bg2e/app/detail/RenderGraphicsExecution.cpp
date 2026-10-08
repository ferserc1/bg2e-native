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
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/ui/UserInterface.hpp>

#ifdef BG2E_LINUX

#include <SDL2/SDL.h>

#else

#include <SDL.h>

#endif

#include <stdexcept>

namespace bg2e {
namespace app {
namespace detail {

class RenderGraphicsExecution : public GraphicsExecution {
public:
    RenderGraphicsExecution() = default;

    // Exception-unwind safety: release any established stage if the owner is
    // destroyed without an explicit cleanup (for example, an exception during
    // run). Cleanup is stage-guarded and never throws from the destructor.
    ~RenderGraphicsExecution() override
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
        return gpu::WindowType::Vulkan;
    }

    void validate(const Application& application) const override
    {
        auto& app = const_cast<Application&>(application);
        if (app.drawDelegate())
        {
            throw std::invalid_argument("MainLoop selected production render, but Application contains a draw::RenderLoopDelegate");
        }
        if (!app.renderDelegate())
        {
            throw std::invalid_argument("MainLoop selected production render, but Application does not contain a render::RenderLoopDelegate");
        }
        if (!app.uiDelegate())
        {
            throw std::invalid_argument("MainLoop selected production render, but Application does not contain a ui::UserInterfaceDelegate");
        }
        if (!app.inputDelegate())
        {
            throw std::invalid_argument("MainLoop selected production render, but Application does not contain an app::InputDelegate");
        }
    }

    void ensureRuntimeAvailable() const override
    {
        // Production execution is always available.
    }

    void initialize(
        SDL_Window* window,
        Application& application,
        ui::UserInterface& userInterface
    ) override
    {
        _engine.init(window);
        _engineInitialized = true;

        int viewportW = 0;
        int viewportH = 0;
        SDL_GetWindowSize(window, &viewportW, &viewportH);
        application.uiDelegate()->setInitialSize(
            static_cast<uint32_t>(viewportW),
            static_cast<uint32_t>(viewportH)
        );

        userInterface.setDelegate(application.uiDelegate());
        userInterface.init(&_engine);
        _userInterface = &userInterface;
        _uiInitialized = true;

        _renderLoop.setDelegate(application.renderDelegate());
        _renderLoop.init(&_engine);
        _renderLoopInitialized = true;

        _renderLoop.renderUICallback([&](VkCommandBuffer cmd, VkImageView targetImageView) {
            _userInterface->draw(cmd, targetImageView);
        });
    }

    void initializeScene() override
    {
        // Initialize the main descriptor set allocator before executing the first frame
        _engine.descriptorSetAllocator().initPool();
        _renderLoop.initScene();
    }

    void requestResize() override
    {
        if (_engineInitialized)
        {
            _engine.updateSwapchainSize();
        }
    }

    void frame(float deltaMilliseconds, bool renderingAllowed) override
    {
        if (renderingAllowed && _engine.newFrame())
        {
            _renderLoop.swapchainResized();
        }

        _renderLoop.setDelta(deltaMilliseconds);

        _userInterface->newFrame();

        if (renderingAllowed)
        {
            _renderLoop.acquireAndPresent();
        }
    }

    void waitIdle() override
    {
        if (_engineInitialized)
        {
            _engine.device().waitIdle();
        }
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
        // Cleanup order matches the production implementation. Engine cleanup
        // closures may still reference UI state, so the UI is released before
        // the engine and the render UI callback capture is cleared last.
        if (_renderLoopInitialized)
        {
            _renderLoop.cleanup();
            _renderLoopInitialized = false;
        }
        if (_uiInitialized)
        {
            _userInterface->cleanup();
            _uiInitialized = false;
        }
        _renderLoop.renderUICallback(nullptr);
        if (_engineInitialized)
        {
            _engine.cleanup();
            _engineInitialized = false;
        }
        _userInterface = nullptr;
    }

protected:
    render::Engine _engine;
    render::RenderLoop _renderLoop;
    ui::UserInterface* _userInterface = nullptr;

    bool _engineInitialized = false;
    bool _uiInitialized = false;
    bool _renderLoopInitialized = false;
};

std::unique_ptr<GraphicsExecution> createRenderGraphicsExecution()
{
    return std::make_unique<RenderGraphicsExecution>();
}

}
}
}
