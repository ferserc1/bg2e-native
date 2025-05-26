//
//  DrawableComponent.hpp

#pragma once

#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/Component.hpp>


namespace bg2e {
namespace scene {

class BG2E_API DrawableComponent : public Component {
public:
    DrawableComponent() = default;
    DrawableComponent(std::shared_ptr<DrawableBase> drw) :_drawable { drw } {}
    DrawableComponent(DrawableBase * drw) :_drawable { std::shared_ptr<DrawableBase>(drw) } {}
    virtual ~DrawableComponent() = default;
    
    std::shared_ptr<DrawableBase> drawable();
    void setDrawable(std::shared_ptr<DrawableBase> drw);

    void draw(
        const glm::mat4& nodeTransform,
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );
    
protected:
    std::shared_ptr<DrawableBase> _drawable;
};

}
}
