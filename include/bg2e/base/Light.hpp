#pragma once

#include <bg2e/base/Color.hpp>

namespace bg2e {
namespace base {

class Light {
public:
    enum LightType {
        TypeOmni = 1,
        TypeSpot = 2,
        TypeDirectional = 3,

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

protected:
    Color _color { 1.0f, 1.0f, 1.0f, 1.0f };
    float _intensity = 1.0f;
    LightType _type = TypeOmni;
};

}
}