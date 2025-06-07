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

    virtual void build(bg2e::render::Engine* engine) = 0;
    virtual void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) = 0;
    virtual void initScene(std::shared_ptr<bg2e::scene::Node> sceneRoot) = 0;
    virtual void resize(VkExtent2D newExtent) = 0;
    virtual void update(float delta) = 0;
    virtual void draw(VkCommandBuffer cmd, uint32_t currentFrame, const bg2e::render::vulkan::Image* depthImage, bg2e::render::vulkan::FrameResources& frameResources) = 0;
    virtual void cleanup() = 0;
};

}
}
