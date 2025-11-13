//
//  GPUSelectionDialog.cpp

#include <bg2e/app/GPUSelectionDialog.hpp>

#include <bg2e/render/vulkan/Instance.hpp>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <iostream>

namespace bg2e::app {

std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>> getAvailableDevices()
{
    std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>> result;
    
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
    
    surface.cleanup();
    instance.cleanup();
    
    SDL_DestroyWindow(dummyWindow);
    dummyWindow = nullptr;
    
    SDL_Quit();
    
    return result;
}

GPUSelectionDialog::GPUSelectionDialog(const std::string & appId)
{

}
    
std::shared_ptr<render::vulkan::PhysicalDeviceProperties> GPUSelectionDialog::run()
{
    std::shared_ptr<render::vulkan::PhysicalDeviceProperties> result;


    
    return result;
}
    
}
