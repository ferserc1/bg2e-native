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

#pragma once

#include <bg2e/scene/Component.hpp>
#include <bg2e/math/base.hpp>

namespace bg2e {
namespace scene {

class BG2E_API TransformComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Transform");
    
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
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;
    

protected:
    glm::mat4 _matrix = glm::mat4(1.0f); // Identity matrix by default
};

}
}
