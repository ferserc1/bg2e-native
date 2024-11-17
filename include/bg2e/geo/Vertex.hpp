#pragma once

#include <glm/glm.hpp>

namespace bg2e {
namespace geo {

struct VertexP {
    glm::vec3 position;
};

struct VertexPN {
    glm::vec3 position;
    glm::vec3 normal;
};

struct VertexPC {
    glm::vec3 position;
    glm::vec4 color;
};

struct VertexPU {
    glm::vec3 position;
    glm::vec2 texCoord0;
};

struct VertexPNU {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord0;
};

struct VertexPNC {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

struct VertexPNUC {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord0;
    glm::vec4 color;
};

struct VertexPNUT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord0;
    glm::vec2 tangent;
};

struct VertexPNUUT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord0;
    glm::vec2 texCoord1;
    glm::vec2 tangent;
};

typedef VertexPNUUT Vertex;

}
}
