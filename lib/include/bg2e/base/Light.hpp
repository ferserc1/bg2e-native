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

#include <bg2e/base/Color.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <memory>

namespace bg2e {
namespace base {

class Light {
public:
    enum LightType {
        TypeOmni = 5,
        TypeSpot = 1,
        TypeDirectional = 4,

        TypeDisabled = 10
    };

    Light() = default;
    virtual ~Light() = default;

    virtual void setColor(const Color& color) { _color = color; }
    virtual const Color& color() const { return _color; }

    virtual void setIntensity(float intensity) { _intensity = intensity; }
    virtual float intensity() const { return _intensity; }

    virtual void setType(LightType type) { _type = type; }
    virtual LightType type() const { return _type; }

    virtual void setSpotAngle(float a) { _spotAngle = a; }
    virtual float spotAngle() const { return _spotAngle; }

    virtual void setSpotCutoff(float a) { _spotCutoff = a; }
    virtual float spotCutoff() const { return _spotCutoff; }

    virtual void setCastShadows(bool cast) { _castShadows = cast; }
    virtual bool castShadows() const { return _castShadows; }

    virtual void setSourceSize(float size) { _sourceSize = size; }
    virtual float sourceSize() const { return _sourceSize; }

    virtual void setShadowSamples(uint32_t samples) { _shadowSamples = samples; }
    virtual uint32_t shadowSamples() const { return _shadowSamples; }

    std::string typeString() const
    {
        switch (_type)
        {
            case TypeOmni:
                return "kTypePoint";
            case TypeSpot:
                return "kTypeSpot";
            case TypeDirectional:
                return "kTypeDirectional";
            default:
                return "kTypeDisabled";
        }
    }

    void deserialize(std::shared_ptr<json::JsonNode> jsonData)
    {
        if (!jsonData || !jsonData->isObject())
            return;

        auto& obj = jsonData->objectValue();

        if (obj.count("color"))
        {
            auto colorNode = obj["color"];
            if (colorNode && colorNode->isVec4())
            {
                _color = colorNode->colorValue();
            }
        }
        if (obj.count("intensity"))
        {
            _intensity = obj["intensity"]->numberValue(_intensity);
        }
        if (obj.count("type"))
        {
            std::string typeStr = obj["type"]->stringValue("");
            if (typeStr == "kTypePoint") _type = TypeOmni;
            else if (typeStr == "kTypeSpot") _type = TypeSpot;
            else if (typeStr == "kTypeDirectional") _type = TypeDirectional;
            else _type = TypeDisabled;
        }
        if (obj.count("spotAngle"))
        {
            _spotAngle = obj["spotAngle"]->numberValue(_spotAngle);
        }
        if (obj.count("spotCutoff"))
        {
            _spotCutoff = obj["spotCutoff"]->numberValue(_spotCutoff);
        }
        if (obj.count("castShadows"))
        {
            _castShadows = obj["castShadows"]->boolValue(_castShadows);
        }
        if (obj.count("sourceSize"))
        {
            _sourceSize = obj["sourceSize"]->numberValue(_sourceSize);
        }
        if (obj.count("shadowSamples"))
        {
            _shadowSamples = static_cast<uint32_t>(
                obj["shadowSamples"]->numberValue(static_cast<int>(_shadowSamples))
            );
        }
    }
    
    std::shared_ptr<json::JsonNode> serialize()
    {
        using namespace bg2e::json;
        return JSON(JsonObject{
            { "color", JSON(_color) },
            { "intensity", JSON(_intensity) },
            { "type", JSON(typeString()) },
            { "spotAngle", JSON(_spotAngle) },
            { "spotCutoff", JSON(_spotCutoff) },
            { "castShadows", JSON(_castShadows) },
            { "sourceSize", JSON(_sourceSize) },
            { "shadowSamples", JSON(static_cast<int>(_shadowSamples)) },
        });
    }
    
protected:
    Color _color { 1.0f, 1.0f, 1.0f, 1.0f };
    float _intensity = 1.0f;
    LightType _type = TypeOmni;
    float _spotAngle = 22.0f;
    float _spotCutoff = 14.0f;
    bool _castShadows = true;
    float _sourceSize = 0.5f;
    uint32_t _shadowSamples = 8;
};

struct LightData
{
    glm::vec3 position;
    float intensity;

    base::Color color;

    glm::vec3 direction;
    base::Light::LightType type;

    float spotAngle;
    float spotCutoff;
    int32_t castShadows;

    float sourceSize;
    int32_t shadowSamples;

    int32_t padding0;
    int32_t padding1;
    int32_t padding2;
};

}
}
