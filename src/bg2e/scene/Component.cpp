//
//  Component.cpp
#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

size_t componentHash(Component * component)
{
    return typeid(*component).hash_code();
}

}
