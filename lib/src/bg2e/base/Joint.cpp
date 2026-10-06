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

#include <bg2e/base/Joint.hpp>
#include <bg2e/json/NodeReader.hpp>

namespace bg2e::base {

std::shared_ptr<Joint> Joint::factory(std::shared_ptr<json::JsonNode> jsonData)
{
    json::ObjectReader reader(jsonData);
    if (reader.getString("type").value_or("") != "LinkJoint")
    {
        return nullptr;
    }

    auto result = std::make_shared<LinkJoint>();
    result->deserialize(jsonData);
    return result;
}

void LinkJoint::setOffset(const glm::vec3& value)
{
    _offset = value;
    calculateTransform();
}

void LinkJoint::setEulerRotation(const glm::vec3& value)
{
    _eulerRotation = value;
    calculateTransform();
}

void LinkJoint::setYaw(float value)
{
    _eulerRotation.x = value;
    calculateTransform();
}

void LinkJoint::setPitch(float value)
{
    _eulerRotation.y = value;
    calculateTransform();
}

void LinkJoint::setRoll(float value)
{
    _eulerRotation.z = value;
    calculateTransform();
}

void LinkJoint::setTransformOrder(LinkTransformOrder value)
{
    _transformOrder = value;
    calculateTransform();
}

void LinkJoint::applyTransform(glm::mat4& destination) const
{
    destination *= _transform;
}

std::shared_ptr<Joint> LinkJoint::clone() const
{
    return std::make_shared<LinkJoint>(*this);
}

void LinkJoint::deserialize(std::shared_ptr<json::JsonNode> jsonData)
{
    json::ObjectReader reader(jsonData);
    if (!reader.isValid()) return;

    if (auto offset = reader.getGlmVec3("offset")) _offset = *offset;

    // Keep the TypeScript behaviour: each deserialization rebuilds Euler angles
    // from the three independent fields, defaulting absent values to zero.
    _eulerRotation = glm::vec3(
        reader.getNumber("yaw").value_or(0.0f),
        reader.getNumber("pitch").value_or(0.0f),
        reader.getNumber("roll").value_or(0.0f)
    );

    // `order` was used by bg2e 1.4. Prefer the current key when both exist.
    if (auto order = reader.getInteger<uint32_t>("transformOrder"); order && *order <= 1)
    {
        _transformOrder = static_cast<LinkTransformOrder>(*order);
    }
    else if (auto order = reader.getInteger<uint32_t>("order"); order && *order <= 1)
    {
        _transformOrder = static_cast<LinkTransformOrder>(*order);
    }
    else
    {
        _transformOrder = LinkTransformOrder::TranslateRotate;
    }

    calculateTransform();
}

std::shared_ptr<json::JsonNode> LinkJoint::serialize() const
{
    using namespace bg2e::json;
    return JSON(JsonObject{
        { "type", JSON("LinkJoint") },
        { "offset", JSON(_offset) },
        { "yaw", JSON(yaw()) },
        { "pitch", JSON(pitch()) },
        { "roll", JSON(roll()) },
        { "transformOrder", JSON(static_cast<uint32_t>(_transformOrder)) }
    });
}

void LinkJoint::calculateTransform()
{
    _transform = glm::mat4(1.0f);
    multiplyTransform(_transform);
}

void LinkJoint::multiplyTransform(glm::mat4& destination) const
{
    switch (_transformOrder)
    {
    case LinkTransformOrder::TranslateRotate:
        destination = glm::translate(destination, _offset);
        multiplyRotation(destination);
        break;
    case LinkTransformOrder::RotateTranslate:
        multiplyRotation(destination);
        destination = glm::translate(destination, _offset);
        break;
    }
}

void LinkJoint::multiplyRotation(glm::mat4& destination) const
{
    destination = glm::rotate(destination, _eulerRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    destination = glm::rotate(destination, _eulerRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    destination = glm::rotate(destination, _eulerRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
}

}
