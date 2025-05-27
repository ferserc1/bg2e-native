#pragma once

#include <bg2e/scene/Component.hpp>
#include <bg2e/math/base.hpp>

namespace bg2e {
namespace scene {

class BG2E_API TransformComponent : public Component {
public:
    static TransformComponent * makeTranslated(float x, float y, float z);
    static TransformComponent * makeRotated(float alpha, float x, float y, float z);
    static TransformComponent * makeScaled(float x, float y, float z);
    
    TransformComponent() = default;
    TransformComponent(const glm::mat4& matrix) : _matrix(matrix) {}
    virtual ~TransformComponent() = default;

    const glm::mat4& matrix() const { return _matrix; }
    void setMatrix(const glm::mat4& matrix) { _matrix = matrix; }
    
    TransformComponent * setTranslation(float x, float y, float z);
    TransformComponent * setRotation(float alpha, float x, float y, float z);
    TransformComponent * setScale(float x, float y, float z);
    TransformComponent * translate(float x, float y, float z);
    TransformComponent * rotate(float alpha, float x, float y, float z);
    TransformComponent * scale(float x, float y, float z);
    
    glm::mat4 worldMatrix();
    glm::mat4 invertedWorldMatrix();

protected:
    glm::mat4 _matrix = glm::mat4(1.0f); // Identity matrix by default
};

}
}
