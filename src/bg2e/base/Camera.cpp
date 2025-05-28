//
//  Camera.cpp

#include <bg2e/base/Camera.hpp>

namespace bg2e::base {

const glm::mat4& Camera::updateProjectionMatrix()
{
    if (_projection.get() != nullptr)
    {
        _projection->apply(_projMatrix);
    }
    
    return _projMatrix;
}

}
