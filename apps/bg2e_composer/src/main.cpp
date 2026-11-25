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

#include <bg2e/base/Log.hpp>
#include "AppDelegate.hpp"
#include "EditorUi.hpp"

#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/draw/EngineConfig.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace bg2e_composer {

class Application final : public bg2e::app::Application {
  public:
    void init(int, char**) override
    {
        auto delegate = std::make_shared<AppDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(std::make_shared<EditorUi>(delegate));
    }
};

}

int main(int argc, char** argv)
{
    struct DiagnosticSession {
        bool active = false;

        ~DiagnosticSession()
        {
            if (active)
            {
                bg2e::base::Log::shutdownDeepDebug();
            }
        }
    } diagnostics;
    try
    {
        bg2e::draw::EngineConfig configuration;
        bool backendSet = false;
        bool deepDebug = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view option(argv[i]);
            if (option == "--help" || option == "-h")
            {
                std::cout << "Usage: bg2e_composer [--backend vulkan|metal] [--deep-debug]\n";
                return 0;
            }
            if (option == "--deep-debug" && !deepDebug)
            {
                deepDebug = true;
                continue;
            }
            if (option != "--backend" || backendSet || i+1 >= argc)
            {
                throw std::invalid_argument("Expected --backend vulkan|metal and/or one --deep-debug flag");
            }
            const std::string_view backend(argv[++i]);
            if (backend == "vulkan")
            {
                configuration.backend = bg2e::gpu::BackendType::Vulkan;
            }
            else if (backend == "metal")
            {
                configuration.backend = bg2e::gpu::BackendType::Metal;
            }
            else
            {
                throw std::invalid_argument("Unsupported backend: expected vulkan or metal");
            }
            backendSet = true;
        }
        if (deepDebug)
        {
            bg2e::base::Log::initializeDeepDebug();
            diagnostics.active = true;
            bg2e_deep_trace("composer", "startup", "backend=" << static_cast<int>(configuration.backend));
        }
        bg2e::app::MainLoop loop("org.bg2e.apps.draw.bg2e-composer");
        bg2e_composer::Application application;
        application.init(argc, argv);
        const auto delegate = std::weak_ptr<bg2e_composer::AppDelegate>(
            std::static_pointer_cast<bg2e_composer::AppDelegate>(application.renderDelegate()));
        if (auto active = delegate.lock())
        {
            active->setQuitCallback([&loop]
            {
                loop.exit();
            });
        }
        loop.setOnExitFunction([delegate]
        {
            auto active = delegate.lock();
            if (!active)
            {
                return true;
            }
            return active->confirmClose();
        });
        loop.initWindowConfig(bg2e::app::WindowConfig::maximized("bg2 engine Composer", true));
        return loop.run(&application, configuration);
    }
    catch (const std::exception& error)
    {
        bg2e_deep_trace("composer", "fatal", error.what());
        std::cerr << "bg2e_composer: " << error.what() << '\n';
        return 1;
    }
}
