#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

namespace bg2e::scene {

TransformComponent * TransformComponent::makeTranslated(float x, float y, float z)
{
    return new TransformComponent(glm::translate( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::makeRotated(float alpha, float x, float y, float z)
{
    return new TransformComponent(glm::rotate( glm::mat4 { 1.0f }, alpha, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::makeScaled(float x, float y, float z)
{
    return new TransformComponent(glm::scale( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::setTranslation(float x, float y, float z) {
    _matrix = glm::translate(glm::mat4{1.0f}, glm::vec3(x, y, z) );
    return this;
}

TransformComponent * TransformComponent::setRotation(float alpha, float x, float y, float z) {
    _matrix = glm::rotate(glm::mat4{1.0f}, alpha, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::setScale(float x, float y, float z) {
    _matrix = glm::scale(glm::mat4{1.0f}, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::translate(float x, float y, float z)
{
    _matrix = glm::translate(_matrix, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::rotate(float alpha, float x, float y, float z)
{
    _matrix = glm::rotate(_matrix, alpha, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::scale(float x, float y, float z)
{
    _matrix = glm::scale(_matrix, glm::vec3(x, y, z));
    return this;
}

glm::mat4 TransformComponent::worldMatrix()
{
    auto owner = ownerNode();
    if (owner == nullptr)
    {
        return matrix();
    }
    TransformVisitor visitor;
    return visitor.getWorldMatrix(owner);
}

glm::mat4 TransformComponent::invertedWorldMatrix()
{
    return glm::inverse(worldMatrix());
}

}
