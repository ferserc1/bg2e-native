
#include <bg2e/render/vulkan/Renderer.hpp>

#include <bg2e/app/Window.hpp>

#include <GLFW/glfw3.h>

#include <iostream>

namespace bg2e {
namespace render {
namespace vulkan {

Renderer::Renderer()
{
    
}

void Renderer::init(const std::string& appName)
{
    _vulkanApi = std::make_unique<VulkanAPI>();
    // TODO: Enable validation layers only if debut
    bool validationLayers = true;

    _vulkanApi->init(validationLayers, appName);
}

void Renderer::bindWindow(app::Window& window)
{
    _vulkanApi->createSurface(window);
}

void Renderer::destroy()
{
    _vulkanApi->destroy();
}

}
}
}
