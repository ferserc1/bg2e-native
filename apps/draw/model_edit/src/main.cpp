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

#include "AppDelegate.hpp"
#include "EditorUi.hpp"

#include <bg2e/app/Application.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/draw/EngineConfig.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace model_edit {

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
    try
    {
        bg2e::draw::EngineConfig configuration;
        bool backendSet = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view option(argv[i]);
            if (option == "--help" || option == "-h")
            {
                std::cout << "Usage: draw_model_edit [--backend vulkan|metal]\n";
                return 0;
            }
            if (option != "--backend" || backendSet || i+1 >= argc)
            {
                throw std::invalid_argument("Expected one --backend vulkan|metal option");
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
        bg2e::app::MainLoop loop("org.bg2e.apps.draw.model-edit");
        model_edit::Application application;
        application.init(argc, argv);
        const auto delegate = std::weak_ptr<model_edit::AppDelegate>(
            std::static_pointer_cast<model_edit::AppDelegate>(application.renderDelegate()));
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
            return !active || active->confirmClose();
        });
        loop.initWindowConfig(bg2e::app::WindowConfig::maximized("bg2e Model Edit", true));
        return loop.run(&application, configuration);
    }
    catch (const std::exception& error)
    {
        std::cerr << "draw_model_edit: " << error.what() << '\n';
        return 1;
    }
}
