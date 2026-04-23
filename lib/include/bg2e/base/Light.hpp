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
    
    std::string typeString() const
    {
        switch (_type)
        {
            case TypeOmni:
                return "OMNI";
            case TypeSpot:
                return "SPOT";
            case TypeDirectional:
                return "DIRECTIONAL";
            default:
                return "DISABLED";
        }
    }

    void deserialize(std::shared_ptr<json::JsonNode>)
    {
    
    }
    
    std::shared_ptr<json::JsonNode> serialize()
    {
        using namespace bg2e::json;
        return JSON(JsonObject{
            { "color", JSON(_color) },
            { "intensity", JSON(_intensity) },
            { "type", JSON(typeString()) }
        });
    }
    
protected:
    Color _color { 1.0f, 1.0f, 1.0f, 1.0f };
    float _intensity = 1.0f;
    LightType _type = TypeOmni;
};

}
}
