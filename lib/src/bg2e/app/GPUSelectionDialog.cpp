//
//  GPUSelectionDialog.cpp

#include <bg2e/app/GPUSelectionDialog.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/SelectableList.hpp>

#include <bg2e/render/vulkan/Instance.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <vector>
#include <iostream>

#include "bg2e/app/PreferencesStore.hpp"

namespace bg2e::app {

void getAvailableDevices(std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>>& result)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error("GPUSelectorDialog: Error initializing SDL");
    }
    
    SDL_Window * dummyWindow = SDL_CreateWindow(
        "bg2e_dummy",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1, 1,
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN
    );
    
    if (!dummyWindow)
    {
        SDL_Quit();
        throw std::runtime_error("GPUSelectorDialog: Error creating dummy SDL Window");
    }

    render::vulkan::Instance instance;
    instance.create(dummyWindow);
    
    render::vulkan::Surface surface;
    surface.create(instance, dummyWindow);

    // TODO: Get devices
    render::vulkan::PhysicalDevice::listSuitableDevices(instance, surface, result);
    surface.cleanup();
    instance.cleanup();
    
    SDL_DestroyWindow(dummyWindow);
    dummyWindow = nullptr;
    
    SDL_Quit();
}

std::shared_ptr<render::vulkan::PhysicalDeviceProperties> showDeviceSelectorUI(
    const std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>>& devices
) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    {
        throw std::runtime_error("GPUSelectorDialog: Error initializing SDL");
    }

#ifdef BG2E_IS_MACOS
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    uint32_t windowWidth = 300;
    uint32_t windowHeight = 200;

    SDL_Window * window = SDL_CreateWindow(
        "Select GPU Device",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load and apply UI scale
    // TODO: Refactor UI Scale apply (bg2e::ui::UserInterface::updateScale())
    auto prefs = bg2e::app::PreferencesStore::instance().preferences("ui");
    auto uiScale = prefs.get("uiScale", 1.0f);
    io.Fonts->Clear();
    auto assets = base::PlatformTools::assetPath() / "DidactGothic-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(assets.string().c_str(), 16.0f);
    io.Fonts->Build();
    auto & style = ImGui::GetStyle();
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
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Initialize frame imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // TODO: Refactor this into UI helpers
        // Begin window without title bar, resize, collapse and moving options, in 0, 0 position
        // with size windowWidth, windowHeight
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
        ImGui::Begin("GPU Devices", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        ui::BasicWidgets::text("Select the GPU device to use:");

        ui::SelectableList::beginList(1);
        
        for (size_t i = 0; i < devices.size(); ++i)
        {
            auto & dev = devices[i];
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
        glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
        glClearColor(0.1f, 0.1f, 0.1f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    std::shared_ptr<render::vulkan::PhysicalDeviceProperties> result;
    if (accepted && selectedDevice < devices.size())
    {
        result = devices[selectedDevice];
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return result;
}

GPUSelectionDialog::GPUSelectionDialog(const std::string & appId)
    : _appId(appId)
{
}
    
std::shared_ptr<render::vulkan::PhysicalDeviceProperties> GPUSelectionDialog::run()
{
    std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>> devices;
    std::shared_ptr<render::vulkan::PhysicalDeviceProperties> result;

    getAvailableDevices(devices);
    
    if (devices.size() == 0)
    {
        throw std::runtime_error("No Vulkan capable devices found");
    }
    else if (devices.size() == 1)
    {
        result = devices[0];
    }
    else {
        result = showDeviceSelectorUI(devices);
    }
    
    return result;
}
    
}
