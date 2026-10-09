#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/DemoWindow.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Window.hpp>

#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace bg2e;

// Shared application state is accessed exclusively on the main thread.
struct Controller {
    app::MainLoop& loop;
    base::Color color{0.12f, 0.18f, 0.28f, 1.0f};
    bool paused = false;
    uint64_t sceneFrames = 0;
    uint64_t uiFrames = 0;
    std::string backend;
};

class SceneDelegate final : public draw::RenderLoopDelegate {
public:
    explicit SceneDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller)) {}

    void render(const draw::FrameContext& context) override
    {
        const auto& color = _controller->color;
        context.commandBuffer.beginRendering(&context.colorTarget);
        context.commandBuffer.clearColor(0, {color.r, color.g, color.b, color.a});
        context.commandBuffer.endRendering();
        ++_controller->sceneFrames;
    }

private:
    std::shared_ptr<Controller> _controller;
};

class UiDelegate final : public ui::UserInterfaceDelegate {
public:
    explicit UiDelegate(std::shared_ptr<Controller> controller)
        : _controller(std::move(controller)) {}

    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Experimental draw controls");
        _window.setPosition(20, 20);
        _window.setSize(380, 300);
        _window.options.noClose = true;
    }

    void drawUI() override
    {
        ++_controller->uiFrames;
        ui::DemoWindow::draw();
        _window.draw([this] {
            auto& state = *_controller;
            ui::Text::text("Backend: " + state.backend);
            ui::Text::text("UI frames: " + std::to_string(state.uiFrames));
            ui::Text::text("Scene updates: " + std::to_string(state.sceneFrames));
            if (ui::Value::colorPicker("Background", state.color))
                state.loop.requestSceneFrame();
            if (ui::Button::checkBox("Pause scene", &state.paused))
            {
                if (state.paused)
                    state.loop.pauseScene({state.color.r, state.color.g, state.color.b, state.color.a});
                else
                    state.loop.resumeScene();
            }
            if (ui::Button::button("Refresh scene")) state.loop.requestSceneFrame();
            if (state.paused)
                ui::Text::text("Scene changes remain pending until resumed.");
        });
    }

private:
    std::shared_ptr<Controller> _controller;
    ui::Window _window;
};

class Application final : public app::Application {
public:
    explicit Application(app::MainLoop& loop)
        : _controller(std::make_shared<Controller>(Controller{loop})) {}

    void init(int argc, char** argv) override
    {
        bool backendSpecified = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument != "--backend=vulkan" && argument != "--backend=metal")
                throw std::invalid_argument("Unknown argument '" + argument +
                    "'. Use --backend=vulkan or --backend=metal.");
            if (backendSpecified)
                throw std::invalid_argument("Specify --backend only once.");
            backendSpecified = true;
            _config.backend = argument == "--backend=metal"
                ? gpu::BackendType::Metal : gpu::BackendType::Vulkan;
        }
#ifndef BG2E_IS_MAC
        if (_config.backend == gpu::BackendType::Metal)
            throw std::invalid_argument("Metal is available only on macOS. Use --backend=vulkan.");
#endif
        _controller->backend = _config.backend == gpu::BackendType::Metal ? "Metal" : "Vulkan";
        setRenderDelegate(std::make_shared<SceneDelegate>(_controller));
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>(_controller));
    }

    const draw::EngineConfig& config() const { return _config; }

private:
    draw::EngineConfig _config;
    std::shared_ptr<Controller> _controller;
};
}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.draw.window-ui");
        Application application(loop);
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e draw: window and UI", 1280, 720));
        return loop.run(&application, application.config());
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_window_ui: " << error.what() << '\n';
        return 1;
    }
}
