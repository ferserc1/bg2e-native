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
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
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

int main(int /*argc*/, char** /*argv*/)
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

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create GPU instance (offscreen — no window)
    auto* instance = backend->instance();
    instance->enableDebugMode(true);
    instance->create();

    // 4. Create offscreen surface
    auto surface = backend->createOffscreenSurface(instance, gpu::Size2D{ 800, 600 });

    // 5. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    auto props = physicalDevice->properties();
    std::cout << "Selected GPU: " << props->name << std::endl;
    std::cout << "  Type:        " << deviceTypeString(props->deviceType) << std::endl;
    std::cout << "  Memory:      " << props->totalHeapMemoryMB << " MB" << std::endl;
    std::cout << "  Ray Tracing: " << (props->rayTracingSupported() ? "Yes" : "No") << std::endl;

    // 6. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    std::cout << "  Graphics queue family: " << device->graphicsQueue().familyIndex() << std::endl;
    std::cout << "  Present queue family:  " << device->presentQueue().familyIndex()  << std::endl;
    std::cout << "  Transfer queue family: " << device->transferQueue().familyIndex() << std::endl;
    std::cout << "  Image count:           " << surface->imageCount() << std::endl;
    std::cout << "  Surface size:          " << surface->width() << "x" << surface->height() << std::endl;

    // 7. Cleanup (reverse order; surface render target depends on device)
    device->waitIdle();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();

    return 0;
}
