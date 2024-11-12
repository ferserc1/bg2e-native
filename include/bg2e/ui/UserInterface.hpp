#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/ui/UserInterfaceDelegate.hpp>
#include <SDL2/SDL.h>


namespace bg2e {
namespace ui {

class BG2E_API UserInterface {
public:
    void init(render::Vulkan *);

    void processEvent(SDL_Event * event);

    void newFrame();
    
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);

    void cleanup();

    inline void setDelegate(std::shared_ptr<UserInterfaceDelegate> delegate) { _delegate = delegate; }

protected:
    render::Vulkan * _vulkan;
    
    VkFence _uiFence;
    VkCommandBuffer _commandBuffer;
    VkCommandPool _commandPool;
    VkDescriptorPool _imguiPool;

    std::shared_ptr<UserInterfaceDelegate> _delegate;
    
    void initCommands();
    void initImGui();
};

}
}
