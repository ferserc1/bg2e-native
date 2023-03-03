
#include <bg2e/base/Light.hpp>

namespace bg2e {
namespace base {

Light::Light()
{

}
 
void Light::deserialize(const std::shared_ptr<tools::JsonNode>& sceneData)
{
    if (sceneData->isObject())
    {
        auto type = (*sceneData)["lightType"]->intValue(LightTypeDirectional);
        if (type == LightTypeDirectional ||
            type == LightTypeSpot ||
            type == LightTypePoint ||
            type == LightTypeDisabled)
        {
            setType(static_cast<LightType>(type));
        }
        
        auto defaultIntensity = _type == LightTypeDirectional ? 1.0f : 300.0f;
        
        setPosition((*sceneData)["position"]->vec3Value(position()));
        setDirection((*sceneData)["direction"]->vec3Value(direction()));
        
        if ((*sceneData)["diffuse"]->isList()) {
            setColor((*sceneData)["diffuse"]->vec4Value(color()));
            setIntensity((*sceneData)["intensity"]->numberValue(1.0f) * defaultIntensity);
        }
        else if ((*sceneData)["color"]->isList()) {
            setColor((*sceneData)["color"]->vec4Value(color()));
            setIntensity((*sceneData)["intensity"]->numberValue(defaultIntensity));
        }
        
        setSpotCutoff((*sceneData)["spotCutoff"]->numberValue(spotCutoff()));
        setSpotExponent((*sceneData)["spotExponent"]->numberValue(spotExponent()));
        setShadowStrength((*sceneData)["shadowStrength"]->numberValue(shadowStrength()));
        
        setProjection((*sceneData)["projection"]->mat4Value());
        
        setCastShadows((*sceneData)["castShadows"]->boolValue(castShadows()));
        
        setShadowBias((*sceneData)["shadowBias"]->numberValue(shadowBias()));
    }
}

void Light::serialize(tools::JsonObject& sceneData)
{
    sceneData["lightType"] = tools::JSON(static_cast<uint32_t>(type()));
    sceneData["position"] = tools::JSON(position());
    sceneData["direction"] = tools::JSON(direction());
    sceneData["color"] = tools::JSON(color());
    sceneData["intensity"] = tools::JSON(intensity());
    sceneData["spotCutoff"] = tools::JSON(spotCutoff());
    sceneData["spotExponent"] = tools::JSON(spotExponent());
    sceneData["shadowStrength"] = tools::JSON(shadowStrength());
    sceneData["projection"] = tools::JSON(projection());
    sceneData["castShadows"] = tools::JSON(castShadows());
    sceneData["shadowBias"] = tools::JSON(shadowBias());
    
}

}
}
