//
//  EnvironmentComponent.hpp
#pragma once

#include <bg2e/scene/Component.hpp>

namespace bg2e {
namespace scene {

class BG2E_API EnvironmentComponent : public Component {
public:
    EnvironmentComponent();
    EnvironmentComponent(const std::string& img);
    virtual ~EnvironmentComponent();
    
    inline void setEnvironmentImage(const std::string& img) { _environmentImage = img; }
    inline const std::string& environmentImage() const { return _environmentImage; }

protected:
    std::string _environmentImage;
    
    // TODO: Other environment parameters
};

}
}
