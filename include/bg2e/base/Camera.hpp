//
//  Camera.hpp
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
