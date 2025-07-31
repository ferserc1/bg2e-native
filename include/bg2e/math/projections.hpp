//
//  projections.hpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <memory>

namespace bg2e {
namespace math {

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 800.0f;
    float height = 600.0f;
    
    Viewport(float x0, float y0, float width0, float height0)
        :x{x0}, y{y0}, width{width0}, height{height0} {}
    Viewport(int32_t x0, int32_t y0, int32_t width0, int32_t height0)
        :x{static_cast<float>(x0)}, y{static_cast<float>(y0)}, width{static_cast<float>(width0)}, height{static_cast<float>(height0)} {}
    Viewport(uint32_t x0, uint32_t y0, uint32_t width0, uint32_t height0)
        :x{static_cast<float>(x0)}, y{static_cast<float>(y0)}, width{static_cast<float>(width0)}, height{static_cast<float>(height0)} {}
    Viewport(float w, float h)
        :x{0}, y{0}, width{w}, height{h} {}
    Viewport(int32_t w, int32_t h)
        :x{0}, y{0}, width{static_cast<float>(w)}, height{static_cast<float>(h)} {}
    Viewport(uint32_t w, uint32_t h)
        :x{0}, y{0}, width{static_cast<float>(w)}, height{static_cast<float>(h)} {}
};

class Projection {
public:
    Projection() :_viewport(800, 600) {}
    virtual ~Projection() {}
    
    virtual void apply(glm::mat4& matrix) = 0;

    inline void setNear(float n) { _near = n; }
    inline void setFar(float f) { _far = f; }
    inline float near() const { return _near; }
    inline float far() const { return _far; }
    
    inline const Viewport& viewport() const { return _viewport; }
    inline Viewport viewport() { return _viewport; }
    inline void setViewport(const Viewport& vp) { _viewport = vp; }
    
    virtual void deserialize(std::shared_ptr<json::JsonNode> jsonData)
    {
    
    }
    
    virtual std::shared_ptr<json::JsonNode> serialize()
    {
        using namespace bg2e::json;
        return JSON(JsonObject({
            { "near", JSON(_near) },
            { "far", JSON(_far) }
            // Viewport is not serialized because is set during render
        }));
    }
    
protected:

    float _near = 0.1f;
    float _far = 100.0f;
    Viewport _viewport;
};

class PerspectiveProjection : public Projection {
public:
    PerspectiveProjection() {}
    virtual ~PerspectiveProjection() {}
    
    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspective(
            glm::radians(_fov),
            _viewport.width / _viewport.height,
            _near,
            _far
        );
    }

    inline void setFov(float fov) { _fov = fov; }
    inline float fov() const { return _fov; }
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData) override{
    
    }
    
    std::shared_ptr<json::JsonNode> serialize() override
    {
        using namespace bg2e::json;
        auto result = Projection::serialize();
        JsonObject & obj = result->objectValue();
        obj["type"] = JSON("PerspectiveProjection");
        obj["fov"] = JSON(_fov);
        return result;
    }
    
protected:

    float _fov = 60.0f;
};

class OpticalProjection : public Projection {
public:
    OpticalProjection() {}
    virtual ~OpticalProjection() {}
    
    void apply(glm::mat4& matrix) override
    {
        float fov = 2.0f * std::atan(_frameSize / (_focalLength * 2.0f));
        matrix = glm::perspective(
            fov,
            _viewport.width / _viewport.height,
            _near,
            _far
        );
    }
    
    inline void setFocalLength(float fl) { _focalLength = fl; }
    inline float focalLength() const { return _focalLength; }
    inline void setFrameSize(float fs) { _frameSize = fs; }
    inline float frameSize() const { return _frameSize; }
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData) override{
    
    }
    
    std::shared_ptr<json::JsonNode> serialize() override
    {
        using namespace bg2e::json;
        auto result = Projection::serialize();
        JsonObject & obj = result->objectValue();
        obj["type"] = JSON("OpticalProjection");
        obj["focalLength"] = JSON(_focalLength);
        obj["frameSize"] = JSON(_frameSize);
        return result;
    }
    
protected:
    float _focalLength = 50.0f;
    float _frameSize = 35.0f;
};


}
}
