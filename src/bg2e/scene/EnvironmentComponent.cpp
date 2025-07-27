//
//  EnvironmentComponent.cpp
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Scene.hpp>

namespace bg2e::scene {

EnvironmentComponent::EnvironmentComponent()
{

}

EnvironmentComponent::EnvironmentComponent(const std::string& img)
    :_environmentImage{img}
{
    _imgHash = std::hash<std::string>{}(img);
}

EnvironmentComponent::EnvironmentComponent(const std::filesystem::path& resourcePath, const std::string& file)
{
    auto fullPath = resourcePath;
    fullPath.append(file);
    _environmentImage = fullPath.string();
    _imgHash = std::hash<std::string>{}(_environmentImage);
}

EnvironmentComponent::~EnvironmentComponent()
{

}

void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{

}

std::shared_ptr<json::JsonNode> EnvironmentComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    std::filesystem::path dstFilePath = basePath;
    std::filesystem::path srcFilePath = _environmentImage;
    auto fileName = srcFilePath.filename();
    dstFilePath += fileName;
    std::filesystem::copy(srcFilePath, dstFilePath, std::filesystem::copy_options::overwrite_existing);
    obj["equirectangularTexture"] = JSON(fileName);
    
    // The following properties are deprecated and are not serialized:
    // - irradianceIntensity
    // - showSkybox
    // - cubemapSize
    // - irradianceMapSize
    // - specularMapSize
    // - specularMapL2Size

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(EnvironmentComponent);

}
