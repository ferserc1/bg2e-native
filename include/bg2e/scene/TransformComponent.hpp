#pragma once

#include <bg2e/scene/Component.hpp>
#include <glm/mat4x4.hpp>

namespace bg2e {
namespace scene {

class BG2E_API TransformComponent : public Component {
public:
    TransformComponent() = default;
    TransformComponent(const glm::mat4& transform) : _transform(transform) {}
    virtual ~TransformComponent() = default;

    const glm::mat4& transform() const { return _transform; }
    void setTransform(const glm::mat4& transform) { _transform = transform; }

protected:
    glm::mat4 _transform = glm::mat4(1.0f); // Identity matrix by default
};

}
}
