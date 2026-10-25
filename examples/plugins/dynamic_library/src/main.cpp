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

#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/DynamicLibrary.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Window.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace bg2e;

class SceneDelegate final : public draw::RenderLoopDelegate {
public:
    void render(const draw::FrameContext& context) override
    {
        context.commandBuffer.beginRendering(&context.colorTarget);
        context.commandBuffer.clearColor(0, {0.12f, 0.18f, 0.28f, 1.0f});
        context.commandBuffer.endRendering();
    }
};

class UiDelegate final : public ui::UserInterfaceDelegate {
public:
    void init(draw::Engine*, ui::UserInterface*) override
    {
        _window.setTitle("Dynamic library example");
        _window.setPosition(20, 20);
        _window.setSize(500, 200);
        _window.options.noClose = true;
    }

    void drawUI() override
    {
        _window.draw([this]
        {
            if (ui::Button::button("Load library"))
            {
                try
                {
                    if (!_library.isLoaded())
                    {
                        auto directory = base::PlatformTools::applicationPath();
#ifdef __APPLE__
                        directory /= "Contents/Resources";
#endif
                        _library = base::DynamicLibrary::load(directory / "plugins" / DYLIB_EXAMPLE_FILENAME);
                    }
                    using GetMessage = const char* (*)();
                    auto getMessage = reinterpret_cast<GetMessage>(_library.getSymbol("getMessage"));
                    if (!getMessage)
                    {
                        throw std::runtime_error("The getMessage symbol is null");
                    }
                    const char* message = getMessage();
                    _message = message ? message : "The plugin returned a null message";
                }
                catch (const std::exception& error)
                {
                    _message = std::string("Error: ") + error.what();
                }
            }
            ui::Text::text(_message);
        });
    }

private:
    ui::Window _window;
    base::DynamicLibrary _library;
    std::string _message = "Click Load library to read the plugin message.";
};

class Application final : public app::Application {
public:
    void init(int, char**) override
    {
        setRenderDelegate(std::make_shared<SceneDelegate>());
        setInputDelegate(std::make_shared<app::InputDelegate>());
        setUiDelegate(std::make_shared<UiDelegate>());
    }
};

}

int main(int argc, char** argv)
{
    try
    {
        bg2e::app::MainLoop loop("org.bg2e.examples.plugins.dynamic-library");
        Application application;
        application.init(argc, argv);
        loop.initWindowConfig(bg2e::app::WindowConfig::withSize("bg2e dynamic library", 1280, 720));
        return loop.run(&application, bg2e::draw::EngineConfig{});
    }
    catch (const std::exception& error)
    {
        std::cerr << "dylib_example_app: " << error.what() << '\n';
        return 1;
    }
}
