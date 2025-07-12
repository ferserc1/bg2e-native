#pragma once

#include <bg2e/scene/Component.hpp>
#include <bg2e/math/base.hpp>

namespace bg2e {
namespace scene {

class BG2E_API TransformComponent : public Component {
public:
    static std::string componentName() { return "Transform"; }
    
    static TransformComponent * makeTranslated(float x, float y, float z);
    static TransformComponent * makeRotated(float alpha, float x, float y, float z);
    static TransformComponent * makeScaled(float xyz);
    static TransformComponent * makeScaled(float x, float y, float z);
    static TransformComponent * makeTranslated(const glm::vec3 &);
    static TransformComponent * makeRotated(float alpha, const glm::vec3& axis);
    static TransformComponent * makeScaled(const glm::vec3& scale);
    
    TransformComponent() = default;
    TransformComponent(const glm::mat4& matrix) : _matrix(matrix) {}
    virtual ~TransformComponent() = default;

    const glm::mat4& matrix() const { return _matrix; }
    void setMatrix(const glm::mat4& matrix) { _matrix = matrix; }
    
    TransformComponent * setTranslation(float x, float y, float z);
    TransformComponent * setRotation(float alpha, float x, float y, float z);
    TransformComponent * setScale(float xyz);
    TransformComponent * setScale(float x, float y, float z);
    TransformComponent * translate(float x, float y, float z);
    TransformComponent * rotate(float alpha, float x, float y, float z);
    TransformComponent * scale(float xyz);
    TransformComponent * scale(float x, float y, float z);
    
    TransformComponent * setTranslation(const glm::vec3& t);
    TransformComponent * setRotation(float alpha, const glm::vec3& axis);
    TransformComponent * setScale(const glm::vec3& scale);
    TransformComponent * translate(const glm::vec3& t);
    TransformComponent * rotate(float alpha, const glm::vec3& axis);
    TransformComponent * scale(const glm::vec3& scale);
    
    glm::mat4 worldMatrix();
    glm::mat4 invertedWorldMatrix();

protected:
    glm::mat4 _matrix = glm::mat4(1.0f); // Identity matrix by default
};

}
}
