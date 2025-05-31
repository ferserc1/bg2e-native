//
//  EnvironmentComponent.cpp
#include <bg2e/scene/EnvironmentComponent.hpp>

namespace bg2e::scene {

EnvironmentComponent::EnvironmentComponent()
{

}

EnvironmentComponent::EnvironmentComponent(const std::string& img)
    :_environmentImage{img}
{

}

EnvironmentComponent::~EnvironmentComponent() {}

}
