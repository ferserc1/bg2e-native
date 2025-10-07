//
//  PickSelectionVisitor.hpp

#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <functional>

namespace bg2e {
namespace manipulation {

class BG2E_API PickSelectionVisitor : public bg2e::scene::NodeVisitor {
public:

    void pick(
        bg2e::scene::Node* sceneRoot,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        VkCommandBuffer cmd,
        VkPipelineLayout layout
    );
    
    void visit(bg2e::scene::Node *);
    void didVisit(bg2e::scene::Node * node);
    
protected:
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    
    glm::mat4 _viewMatrix;
    glm::mat4 _projMatrix;
    
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;
};

}
}
