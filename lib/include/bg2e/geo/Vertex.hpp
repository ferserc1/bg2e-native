/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_LEFT_HANDED 1
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
    glm::vec3 tangent;
};

struct VertexPNUUT {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord0;
    glm::vec2 texCoord1;
    glm::vec3 tangent;
};

// Default vertex type
typedef VertexPNUUT Vertex;

}
}
