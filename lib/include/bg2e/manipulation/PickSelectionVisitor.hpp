//
//  PickSelectionVisitor.hpp

#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <functional>
#include <stack>
#include <unordered_map>

namespace bg2e {
namespace manipulation {

class BG2E_API PickSelectionVisitor : public bg2e::scene::NodeVisitor {
public:
    struct SubmeshLookupData
    {
        bg2e::scene::Node * node;
        uint32_t submeshIndex;
    };

    void pick(
        bg2e::scene::Node* sceneRoot,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        VkCommandBuffer cmd,
        VkPipelineLayout layout
    );
    
    void visit(bg2e::scene::Node *) override;
    void didVisit(bg2e::scene::Node * node) override;

    SubmeshLookupData * findObject(uint32_t identifier);

protected:
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    
    glm::mat4 _viewMatrix;
    glm::mat4 _projMatrix;
    
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;

    std::unordered_map<uint32_t, SubmeshLookupData> _lookupNodes;
};

}
}
