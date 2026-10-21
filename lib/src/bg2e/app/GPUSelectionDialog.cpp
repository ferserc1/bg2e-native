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

#include <bg2e/app/GPUSelectionDialog.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <bg2e/ui/SelectableList.hpp>

#include <bg2e/ui/Text.hpp>

#include <SDL2/SDL.h>
#include <bg2e/gpu/StandaloneGuiWindow.hpp>

#include <imgui.h>

#include <vector>
#include <stdexcept>

#include <bg2e/app/PreferencesStore.hpp>

namespace bg2e::app {

namespace {
// The bridge is destroyed first, while this context is still current.
struct GuiContextScope {
    ImGuiContext* previous = ImGui::GetCurrentContext();
    ImGuiContext* context = nullptr;

    GuiContextScope()
    {
        IMGUI_CHECKVERSION();
        context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);
    }
    ~GuiContextScope()
    {
        ImGui::DestroyContext(context);
        ImGui::SetCurrentContext(previous);
    }
};
}

std::shared_ptr<gpu::PhysicalDeviceProperties> showDeviceSelectorUI(
    const std::vector<std::shared_ptr<gpu::PhysicalDeviceProperties>>& devices)
{
    GuiContextScope guiContext;
    const uint32_t windowWidth = 300;
    const uint32_t windowHeight = 200;
    gpu::StandaloneGuiWindow window("Select GPU Device", {windowWidth, windowHeight});
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    // Load and apply UI scale
    // TODO: Refactor UI Scale apply (bg2e::ui::UserInterface::updateScale())
    auto prefs = bg2e::app::PreferencesStore::instance().preferences("ui");
    auto uiScale = prefs.get("uiScale", 1.0f);
    io.Fonts->Clear();
    auto assets = base::PlatformTools::assetPath() / "DidactGothic-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(assets.string().c_str(), 16.0f);
    io.Fonts->Build();
    auto& style = ImGui::GetStyle();
    style.ScaleAllSizes(uiScale);
    io.FontGlobalScale = uiScale;

    bool running = true;
    bool accepted = false;

    std::vector<bool> selectionStates(devices.size(), false);
    size_t selectedDevice = -1;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            window.processEvent(event);
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Initialize frame imgui
        window.prepareFrame();
        ImGui::NewFrame();

        // TODO: Refactor this into UI helpers
        // Begin window without title bar, resize, collapse and moving options, in 0, 0 position
        // with size windowWidth, windowHeight
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
        ImGui::Begin("GPU Devices", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        ui::Text::text("Select the GPU device to use:");

        if (ui::SelectableList::beginList(1))
        {
            for (size_t i = 0; i < devices.size(); ++i)
            {
                auto& dev = devices[i];
                bool selected = selectionStates[i];
                if (ui::SelectableList::item(dev->name + "##" + std::to_string(i), selected))
                {
                    for (size_t j = 0; j < selectionStates.size(); ++j)
                    {
                        selectionStates[j] = (j == i);
                    }
                    selectedDevice = i;
                }
            }
            ui::SelectableList::endList();
        }

        if (ImGui::Button("Accept"))
        {
            accepted = true;
            running = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            running = false;
        }

        ImGui::End();

        ImGui::Render();
        window.present();
    }

    std::shared_ptr<gpu::PhysicalDeviceProperties> result;
    if (accepted && selectedDevice < devices.size())
    {
        result = devices[selectedDevice];
    }

    return result;
}

GPUSelectionDialog::GPUSelectionDialog(const std::string& appId)
    : _appId(appId)
{
}

std::shared_ptr<gpu::PhysicalDeviceProperties> GPUSelectionDialog::run(const std::vector<std::shared_ptr<gpu::PhysicalDeviceProperties>>& devices)
{
    std::shared_ptr<gpu::PhysicalDeviceProperties> result;

    if (devices.size() == 0)
    {
        throw std::runtime_error("No suitable GPU devices supplied");
    }
    else if (devices.size() == 1)
    {
        result = devices[0];
    }
    else
    {
        result = showDeviceSelectorUI(devices);
    }

    return result;
}

}
