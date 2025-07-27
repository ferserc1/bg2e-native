#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/TransformVisitor.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

namespace bg2e::scene {

TransformComponent * TransformComponent::makeTranslated(float x, float y, float z)
{
    return new TransformComponent(glm::translate( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::makeTranslated(const glm::vec3& t)
{
    return new TransformComponent(glm::translate( glm::mat4 { 1.0f }, t ));
}

TransformComponent * TransformComponent::makeRotated(float alpha, float x, float y, float z)
{
    return new TransformComponent(glm::rotate( glm::mat4 { 1.0f }, alpha, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::makeRotated(float alpha, const glm::vec3& axis)
{
    return new TransformComponent(glm::rotate( glm::mat4 { 1.0f }, alpha, axis ));
}

TransformComponent * TransformComponent::makeScaled(float xyz)
{
    return new TransformComponent(glm::scale( glm::mat4 { 1.0f }, glm::vec3(xyz) ));
}

TransformComponent * TransformComponent::makeScaled(float x, float y, float z)
{
    return new TransformComponent(glm::scale( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

TransformComponent * TransformComponent::makeScaled(const glm::vec3& scale)
{
    return new TransformComponent(glm::scale( glm::mat4 { 1.0f }, scale ));
}

TransformComponent * TransformComponent::setTranslation(float x, float y, float z) {
    _matrix = glm::translate(glm::mat4{1.0f}, glm::vec3(x, y, z) );
    return this;
}

TransformComponent * TransformComponent::setRotation(float alpha, float x, float y, float z) {
    _matrix = glm::rotate(glm::mat4{1.0f}, alpha, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::setScale(float xyz) {
    _matrix = glm::scale(glm::mat4{1.0f}, glm::vec3(xyz));
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

TransformComponent * TransformComponent::scale(float xyz)
{
    _matrix = glm::scale(_matrix, glm::vec3(xyz));
    return this;
}

TransformComponent * TransformComponent::scale(float x, float y, float z)
{
    _matrix = glm::scale(_matrix, glm::vec3(x, y, z));
    return this;
}

TransformComponent * TransformComponent::setTranslation(const glm::vec3& t) {
    _matrix = glm::translate(glm::mat4{1.0f}, t );
    return this;
}

TransformComponent * TransformComponent::setRotation(float alpha, const glm::vec3& axis) {
    _matrix = glm::rotate(glm::mat4{1.0f}, alpha, axis);
    return this;
}

TransformComponent * TransformComponent::setScale(const glm::vec3& scale) {
    _matrix = glm::scale(glm::mat4{1.0f}, scale);
    return this;
}

TransformComponent * TransformComponent::translate(const glm::vec3& t)
{
    _matrix = glm::translate(_matrix, t);
    return this;
}

TransformComponent * TransformComponent::rotate(float alpha, const glm::vec3& axis)
{
    _matrix = glm::rotate(_matrix, alpha, axis);
    return this;
}

TransformComponent * TransformComponent::scale(const glm::vec3& scale)
{
    _matrix = glm::scale(_matrix, scale);
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

void TransformComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{

}

std::shared_ptr<json::JsonNode> TransformComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    obj["transformMatrix"] = JSON(_matrix);
    
    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(TransformComponent);
    
}
