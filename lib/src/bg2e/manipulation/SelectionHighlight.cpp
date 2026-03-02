
#include <bg2e/manipulation/SelectionHighlight.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/Node.hpp>

#include <iostream>

namespace bg2e::manipulation {

void SelectionHighlight::draw(
    bg2e::scene::Node * sceneRoot,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    VkCommandBuffer cmd
) {
    _viewMatrix = viewMatrix;
    _projectionMatrix = projMatrix;
    _cmdBuffer = cmd;
    sceneRoot->accept(this);
}

void SelectionHighlight::visit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    auto selectable = node->getComponent<bg2e::manipulation::SelectableComponent>();
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();

    if (trx)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * trx->matrix();
    }

    if (selectable && drawable)
    {
        auto submeshes = drawable->drawable()->submeshesCount();
        for (uint32_t i = 0; i < submeshes; ++i)
        {
            if (selectable->isSelected(i))
            {
                auto pos = glm::vec3(_currentTransform[3]);
                std::cout << "Selected: " << drawable->drawable()->submeshName(i) <<
                    " at position " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            }
        }
    }
}

void SelectionHighlight::didVisit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    if (trx)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

}
