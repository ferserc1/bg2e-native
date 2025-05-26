//
//  DrawableComponent.cpp
#include <bg2e/scene/DrawableComponent.hpp>

#include <iostream>

namespace bg2e::scene {

std::shared_ptr<DrawableBase> DrawableComponent::drawable()
{
    return _drawable;
}

void DrawableComponent::setDrawable(std::shared_ptr<DrawableBase> drw)
{
    _drawable = drw;
}

void DrawableComponent::draw(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb,
    VkPipelineBindPoint bp
) {
    std::cout << "Draw drawable" << std::endl;
}
}
