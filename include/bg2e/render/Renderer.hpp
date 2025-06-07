#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {

class Renderer {
public:
    virtual ~Renderer() = default;

    inline VkFormat colorAttachmentFormat() const { return _colorAttachmentFormat; }
    inline VkFormat depthAttachmentFormat() const { return _depthAttachmentFormat; }
    inline const bg2e::render::ColorAttachments* colorAttachments() const { return _colorAttachments.get(); }
    inline bg2e::render::ColorAttachments* colorAttachments() { return _colorAttachments.get(); }
    
    virtual void build(
        bg2e::render::Engine* engine,
        VkFormat colorAttachmentFormat = VK_FORMAT_R8G8B8A8_UNORM,
        VkFormat depthAttachmentFormat = VK_FORMAT_D32_SFLOAT
    ) = 0;
    
    virtual void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) = 0;
    virtual void initScene(std::shared_ptr<bg2e::scene::Node> sceneRoot) = 0;
    virtual void resize(VkExtent2D newExtent) = 0;
    virtual void update(float delta) = 0;
    virtual void draw(VkCommandBuffer cmd, uint32_t currentFrame, const bg2e::render::vulkan::Image* depthImage, bg2e::render::vulkan::FrameResources& frameResources) = 0;
    virtual void cleanup() = 0;

protected:
    VkFormat _colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat _depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    std::unique_ptr<bg2e::render::ColorAttachments> _colorAttachments;
};

}
}
