#ifndef bg2e_base_light_hpp
#define bg2e_base_light_hpp

#include <memory>

#include <bg2e/export.hpp>
#include <bg2e/tools/Json.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace bg2e {
namespace base {

typedef enum {
    LightTypeDisabled = 10,
    LightTypeDirectional = 4,
    LightTypeSpot = 1,
    LightTypePoint = 5
} LightType;

class BG2E_EXPORT Light
{
public:
	Light();
    
    Light(Light&&) = delete;
    Light& operator=(Light&&) = delete;
    
    LightType type() const { return _type; }
    const glm::vec3& direction() const { return _direction; }
    const glm::vec3& position() const { return _position; }
    const glm::vec4& color() const { return _color; }
    float intensity() const { return _intensity; }
    float spotCutoff() const { return _spotCutoff; }
    float spotExponent() const { return _spotExponent; }
    float shadowStrength() const { return _shadowStrength; }
    bool castShadows() const { return _castShadows; }
    float shadowBias() const { return _shadowBias; }
    const glm::mat4& projection() const { return _projection; }

    inline void setType(LightType v) { _type = v; }
    inline void setDirection(const glm::vec3& v) { _direction = v; }
    inline void setPosition(const glm::vec3& v) { _position = v; }
    inline void setColor(const glm::vec4& v) { _color = v; }
    inline void setIntensity(float v) { _intensity = v; }
    inline void setSpotCutoff(float v) { _spotCutoff = v; }
    inline void setSpotExponent(float v) { _spotExponent = v; }
    inline void setShadowStrength(float v) { _shadowStrength = v; }
    inline void setCastShadows(bool v) { _castShadows = v; }
    inline void setShadowBias(float v) { _shadowBias = v; }
    inline void setProjection(const glm::mat4& v) { _projection = v; }

    std::shared_ptr<Light> clone()
    {
        auto copy = std::make_shared<Light>();
        *copy = *this;
        return copy;
    }
    
    Light& operator=(const Light& other)
    {
        _type = other._type;
        _direction = other._direction;
        _position = other._position;
        _color = other._color;
        _intensity = other._intensity;
        _spotCutoff = other._spotCutoff;
        _spotExponent = other._spotExponent;
        _shadowStrength = other._shadowStrength;
        _castShadows = other._castShadows;
        _shadowBias = other._shadowBias;
        return *this;
    }

    void deserialize(const std::shared_ptr<tools::JsonNode>&);
    
    void serialize(tools::JsonObject&);
    
    
protected:
	bool _enabled;

    LightType _type = LightTypeDirectional;
    
    
    glm::vec3 _direction = glm::vec3{0.0f, 0.0f, -1.0f};
    glm::vec3 _position = glm::vec3{0.0f, 0.0f, 0.0f};
    
    glm::vec4 _color = glm::vec4{0.9f, 0.9f, 0.9f, 1.0f};
    float _intensity = 300.0f;
    float _spotCutoff = 20.0f;
    float _spotExponent = 30.0f;
    float _shadowStrength = 0.7f;
    bool _castShadows = true;
    float _shadowBias = 0.00002f;
    
    glm::mat4 _projection = glm::perspective(glm::radians(45.0f), 1.33f, 0.1f, 100.0f);
};

}
}
#endif
