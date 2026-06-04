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
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <iostream>

static const char* deviceTypeString(bg2e::gpu::PhysicalDeviceProperties::DeviceType type)
{
    switch (type)
    {
        case bg2e::gpu::PhysicalDeviceProperties::DiscreteGPU:   return "Discrete GPU";
        case bg2e::gpu::PhysicalDeviceProperties::IntegratedGPU: return "Integrated GPU";
        case bg2e::gpu::PhysicalDeviceProperties::VirtualGPU:    return "Virtual GPU";
        case bg2e::gpu::PhysicalDeviceProperties::CPU:           return "CPU";
        default:                                                  return "Unknown";
    }
}

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend to query window type
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. SDL init + create window based on backend window type
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GPU Device Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 4. Create GPU instance
    auto* instance = backend->instance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create surface
    auto surface = backend->createWindowSurface();
    surface->create(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: " << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
    std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;

    // 8. Event loop
    bool running = true;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
        }
    }

    // 9. Cleanup (reverse order)
    device->cleanup();
    surface->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
