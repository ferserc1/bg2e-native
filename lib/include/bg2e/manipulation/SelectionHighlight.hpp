
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <stack>

namespace bg2e {
namespace manipulation {

class BG2E_API SelectionHighlight : public bg2e::scene::NodeVisitor{
public:

    void draw(
        bg2e::scene::Node * sceneRoot,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        VkCommandBuffer cmd
    );
    void visit(bg2e::scene::Node * node) override;
    void didVisit(bg2e::scene::Node * node) override;

protected:
    glm::mat4 _viewMatrix { 1.0f };
    glm::mat4 _projectionMatrix { 1.0f };
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;

    VkCommandBuffer _cmdBuffer = VK_NULL_HANDLE;
};

}
}