//
//  DrawVisitor.hpp
#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <stack>

namespace bg2e {
namespace scene {

class BG2E_API DrawVisitor : public bg2e::scene::NodeVisitor {
public:
    DrawVisitor() {}
    
    void draw(
        bg2e::scene::Node* sceneRoot,
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        bg2e::scene::DrawableBase::DrawFunction cb
    );
    
    void visit(bg2e::scene::Node * node);
    
    void didVisit(bg2e::scene::Node * node);

protected:
    bg2e::scene::DrawableBase::DrawFunction _drawFunction;
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;
};

}
}
