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
#include <bg2e/math/projections.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <memory>

namespace bg2e {
namespace base {

class BG2E_API Camera {
public:
    inline void setProjection(math::Projection * p) { _projection = std::shared_ptr<math::Projection>(p); }
    inline void setProjection(std::shared_ptr<math::Projection> p) { _projection = p; }
    inline math::Projection * projection() { return _projection.get(); }
    template <class T>
    inline T * projection() { return dynamic_cast<T*>(_projection.get()); }
    
    inline const glm::mat4& projectionMatrix() const { return _projMatrix; }
    
    const glm::mat4& updateProjectionMatrix();
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData)
    {
        if (!jsonData || !jsonData->isObject())
            return;

        auto& obj = jsonData->objectValue();
        if (obj.count("projection"))
        {
            auto projData = obj["projection"];
            if (projData && projData->isObject())
            {
                auto& projObj = projData->objectValue();
                std::string type = projObj.count("type") ?
                    projObj["type"]->stringValue("") : "";

                if (type == "PerspectiveProjection")
                {
                    auto proj = std::make_shared<math::PerspectiveProjection>();
                    proj->deserialize(projData);
                    _projection = proj;
                }
                else if (type == "OpticalProjection")
                {
                    auto proj = std::make_shared<math::OpticalProjection>();
                    proj->deserialize(projData);
                    _projection = proj;
                }
            }
        }
        else if (obj.count("projectionMatrix"))
        {
            auto matNode = obj["projectionMatrix"];
            if (matNode && matNode->isMat4())
            {
                _projMatrix = matNode->glmMat4Value();
            }
        }
    }
    
    std::shared_ptr<json::JsonNode> serialize()
    {
        using namespace bg2e::json;
        if (_projection.get())
        {
            return JSON(JsonObject{
                { "projection", _projection->serialize() }
            });
        }
        else
        {
            return JSON(JsonObject{
                { "projectionMatrix", JSON(_projMatrix) }
            });
        }
    }
    
protected:
    std::shared_ptr<math::Projection> _projection;
    glm::mat4 _projMatrix;
};

}
}
