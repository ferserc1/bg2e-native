/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/TransformVisitor.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

#include <cmath>

namespace bg2e::scene {

std::unique_ptr<TransformComponent> TransformComponent::makeTranslated(float x, float y, float z)
{
    return std::make_unique<TransformComponent>(glm::translate( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeTranslated(const glm::vec3& t)
{
    return std::make_unique<TransformComponent>(glm::translate( glm::mat4 { 1.0f }, t ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeRotated(float alpha, float x, float y, float z)
{
    return std::make_unique<TransformComponent>(glm::rotate( glm::mat4 { 1.0f }, alpha, glm::vec3(x, y, z) ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeRotated(float alpha, const glm::vec3& axis)
{
    return std::make_unique<TransformComponent>(glm::rotate( glm::mat4 { 1.0f }, alpha, axis ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeScaled(float xyz)
{
    return std::make_unique<TransformComponent>(glm::scale( glm::mat4 { 1.0f }, glm::vec3(xyz) ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeScaled(float x, float y, float z)
{
    return std::make_unique<TransformComponent>(glm::scale( glm::mat4 { 1.0f }, glm::vec3(x, y, z) ));
}

std::unique_ptr<TransformComponent> TransformComponent::makeScaled(const glm::vec3& scale)
{
    return std::make_unique<TransformComponent>(glm::scale( glm::mat4 { 1.0f }, scale ));
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

TransformComponent * TransformComponent::setIdentity()
{
    _matrix = glm::identity<glm::mat4>();
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

glm::vec3 TransformComponent::extractTranslation(const glm::mat4& m)
{
    return glm::vec3(m[3]);
}

glm::mat3 TransformComponent::extractRotation(const glm::mat4& m)
{
    float sx = glm::length(glm::vec3(m[0]));
    float sy = glm::length(glm::vec3(m[1]));
    float sz = glm::length(glm::vec3(m[2]));
    glm::mat3 r;
    r[0] = glm::vec3(m[0]) / sx;
    r[1] = glm::vec3(m[1]) / sy;
    r[2] = glm::vec3(m[2]) / sz;
    return r;
}

glm::vec3 TransformComponent::extractScale(const glm::mat4& m)
{
    return glm::vec3(
        glm::length(glm::vec3(m[0])),
        glm::length(glm::vec3(m[1])),
        glm::length(glm::vec3(m[2]))
    );
}

glm::mat4 TransformComponent::recompose(const glm::mat3& rotation, const glm::vec3& scale, const glm::vec3& translation)
{
    glm::mat4 result(1.0f);
    result[0] = glm::vec4(rotation[0] * scale.x, 0.0f);
    result[1] = glm::vec4(rotation[1] * scale.y, 0.0f);
    result[2] = glm::vec4(rotation[2] * scale.z, 0.0f);
    result[3] = glm::vec4(translation, 1.0f);
    return result;
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

void TransformComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& /* basePath */, [[maybe_unused]] render::Engine& engine)
{
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    if (obj.count("transformMatrix") && obj["transformMatrix"]->isMat4())
    {
        _matrix = obj["transformMatrix"]->glmMat4Value();
    }
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
