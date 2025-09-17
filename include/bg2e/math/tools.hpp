//
//  tools.hpp

#pragma once

#include <glm/glm.hpp>

namespace bg2e {
namespace math {

struct BasisVectors {
    BasisVectors() {}
    BasisVectors(const glm::mat4 & mat) {
        // TODO: Review this code
        right = (mat * glm::vec4{ 1, 0, 0, 1 });
        front = (mat * glm::vec4{ 0, 0, 1, 1 });
        up = (mat * glm::vec4{ 0, 1, 0, 1 });
    }
    
    glm::vec3 right;
    glm::vec3 front;
    glm::vec3 up;
};

}
}
