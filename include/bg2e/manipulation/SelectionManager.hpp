//
//  SelectionManager.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace manipulation {

class SelectionItem {
    std::shared_ptr<scene::Drawable> mesh;
    uint32_t submesh;
};

class BG2E_API SelectionManager {
public:
    SelectionManager(render::Engine * engine);
    virtual ~SelectionManager() = default;

    void init();

    bool pick(scene::Node * rootNode, const math::Viewport & vp, uint32_t x, uint32_t y);

protected:
    render::Engine * _engine;
    
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
    std::shared_ptr<render::vulkan::Image> _image;
    
    struct PushConstantData {
        glm::mat4 mvp;
        uint32_t identifier;
    };
    
    void createImage();
    void createPipeline();
};

}
}
