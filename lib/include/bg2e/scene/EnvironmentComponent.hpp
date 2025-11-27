//
//  EnvironmentComponent.hpp
#pragma once

#include <bg2e/scene/Component.hpp>

#include <string>
#include <filesystem>


namespace bg2e {
namespace scene {

class BG2E_API EnvironmentComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Environment");

    EnvironmentComponent();
    EnvironmentComponent(const std::string& img);
    EnvironmentComponent(const std::filesystem::path& resourcePath, const std::string& file);
    virtual ~EnvironmentComponent();
    
    inline void setEnvironmentImage(const std::string& img)
    {
        _environmentImage = img;
        _imgHash = std::hash<std::string>{}(img);
    }
    
    inline void setEnvironmentImage(const std::filesystem::path& resourcePath, const std::string& img)
    {
        auto fullPath = resourcePath;
        fullPath.append(img);
        setEnvironmentImage(fullPath.string());
    }
    inline const std::string& environmentImage() const { return _environmentImage; }
    
    inline size_t imgHash() const { return _imgHash; }
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    std::string _environmentImage;
    size_t _imgHash = 0;
    

    
    // TODO: Other environment parameters
};

}
}
