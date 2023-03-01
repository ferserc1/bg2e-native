
#include <bg2e/base/Environment.hpp>

namespace bg2e {
namespace base {

Environment::Environment()
{
}

void Environment::deserialize(const std::shared_ptr<tools::JsonNode>& sceneData)
{
    if (sceneData->isObject()) {
        setEquirectangularTexture((*sceneData)["equirectangularTexture"]->stringValue(
            std::string(equirectangularTexture())
        ));
        setIrradianceIntensity((*sceneData)["irradianceIntensity"]->numberValue(irradianceIntensity()));
        setShowSkybox((*sceneData)["showSkybox"]->boolValue(showSkybox()));
        setCubemapSize((*sceneData)["cubemapSize"]->intValue(cubemapSize()));
        setIrradianceMapSize((*sceneData)["irradianceMapSize"]->intValue(irradianceMapSize()));
        setSpecularMapSize((*sceneData)["specularMapSize"]->intValue(specularMapSize()));
        setSpecularMapL2Size((*sceneData)["specularMapL2Size"]->intValue(specularMapL2Size()));
    }
}

void Environment::serialize(tools::JsonObject& sceneData)
{
    sceneData["equirectangularTexture"] = tools::JSON(equirectangularTexture());
    sceneData["irradianceIntensity"] = tools::JSON(irradianceIntensity());
    sceneData["showSkybox"] = tools::JSON(showSkybox());
    sceneData["cubemapSize"] = tools::JSON(cubemapSize());
    sceneData["irradianceMapSize"] = tools::JSON(irradianceMapSize());
    sceneData["specularMapSize"] = tools::JSON(specularMapSize());
    sceneData["specularMapL2Size"] = tools::JSON(specularMapL2Size());
}

}
}
