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

#include <bg2e/common.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <memory>

namespace bg2e::base {

class BG2E_API Joint {
public:
    virtual ~Joint() = default;

    static std::shared_ptr<Joint> factory(std::shared_ptr<json::JsonNode> jsonData);

    const glm::mat4& transform() const { return _transform; }

    virtual void applyTransform(glm::mat4& destination) const = 0;
    virtual std::shared_ptr<Joint> clone() const = 0;
    virtual void deserialize(std::shared_ptr<json::JsonNode> jsonData) = 0;
    virtual std::shared_ptr<json::JsonNode> serialize() const = 0;

protected:
    glm::mat4 _transform { 1.0f };
};

enum class LinkTransformOrder : uint32_t {
    RotateTranslate = 0,
    TranslateRotate = 1
};

class BG2E_API LinkJoint : public Joint {
public:
    LinkJoint() = default;

    const glm::vec3& offset() const { return _offset; }
    void setOffset(const glm::vec3& value);

    const glm::vec3& eulerRotation() const { return _eulerRotation; }
    void setEulerRotation(const glm::vec3& value);

    float yaw() const { return _eulerRotation.x; }
    void setYaw(float value);
    float pitch() const { return _eulerRotation.y; }
    void setPitch(float value);
    float roll() const { return _eulerRotation.z; }
    void setRoll(float value);

    LinkTransformOrder transformOrder() const { return _transformOrder; }
    void setTransformOrder(LinkTransformOrder value);

    void applyTransform(glm::mat4& destination) const override;
    std::shared_ptr<Joint> clone() const override;
    void deserialize(std::shared_ptr<json::JsonNode> jsonData) override;
    std::shared_ptr<json::JsonNode> serialize() const override;

protected:
    void calculateTransform();
    void multiplyTransform(glm::mat4& destination) const;
    void multiplyRotation(glm::mat4& destination) const;

    glm::vec3 _offset { 0.0f };
    glm::vec3 _eulerRotation { 0.0f };
    LinkTransformOrder _transformOrder = LinkTransformOrder::TranslateRotate;
};

}
