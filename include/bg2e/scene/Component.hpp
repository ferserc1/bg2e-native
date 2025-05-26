//
//  Component.hpp
#pragma once

#include <bg2e/common.hpp>

#include <string>

namespace bg2e {
namespace scene {

class Component {
public:
    
    virtual ~Component() = default;
    
};

extern BG2E_API size_t componentHash(Component * comp);

}
}
