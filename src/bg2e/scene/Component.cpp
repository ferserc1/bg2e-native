//
//  Component.cpp
#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>

namespace bg2e::scene {

Scene * Component::scene()
{
    return _owner != nullptr ? _owner->scene() : nullptr;
}

std::string componentName(Component * component)
{
    return typeid(*component).name();
}

}
