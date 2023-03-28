
#include <bg2e/render/vulkan/Renderer.hpp>
#include <bg2e/render/vulkan/CommandQueue.hpp>

#include <bg2e/app/Window.hpp>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <iostream>

namespace bg2e {
namespace render {
namespace vulkan {

Renderer::Renderer()
{
    
}

void Renderer::init(const std::string& appName)
{
    _vulkanApi = std::make_shared<VulkanAPI>();
    _appName = appName;
}

void Renderer::bindWindow(app::Window& window)
{
    // TODO: Enable validation layers only if debug
    bool validationLayers = true;

    _vulkanApi->init(validationLayers, _appName, window);
}

void Renderer::resize(uint32_t w, uint32_t h)
{
    _vulkanApi->setResized();
    _windowSize.width = w;
    _windowSize.height = h;
}

void Renderer::update(float delta)
{
    
}

void Renderer::drawFrame()
{
    if (_drawFunction)
    {
        int32_t imageIndex = _vulkanApi->beginFrame();
        if (imageIndex >= 0)
        {
            auto commandBuffer = _vulkanApi->commandBuffer();
            
            commandBuffer.begin(vk::CommandBufferBeginInfo{});
            
            _drawFunction(commandQueue());
            
            commandBuffer.end();
            
            _vulkanApi->endFrame(imageIndex);
        }
    }
    
}

void Renderer::destroy()
{
    _vulkanApi->destroy();
}

render::CommandQueue* Renderer::commandQueue()
{
    if (_commandQueue.get() == nullptr)
    {
        _commandQueue = std::make_shared<CommandQueue>(this);
    }
    return _commandQueue.get();
}

}
}
}
