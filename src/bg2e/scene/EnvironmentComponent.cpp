//
//  EnvironmentComponent.cpp
#include <bg2e/scene/EnvironmentComponent.hpp>
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

}
