//
//  tools.hpp

#pragma once

#include <glm/glm.hpp>

namespace bg2e {
namespace math {

struct BasisVectors {
    BasisVectors() {}
    BasisVectors(const glm::mat4 & mat, bool isViewMartix) {
        // glm::mat4 It is column-major, so each column represents an axis.
        right = glm::normalize(glm::vec3(mat[0]));
        up    = glm::normalize(glm::vec3(mat[1]));
        forward = glm::normalize(glm::vec3(mat[2]));
        
        if (isViewMartix)
        {
            forward = -forward;
        }
    }
    
    glm::vec3 right;
    glm::vec3 forward;
    glm::vec3 up;
};


}
}

