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

#include <bg2e/common.hpp>
#include <bg2e/geo/AABoundingBox.hpp>
#include <bg2e/scene/Mesh.hpp>

namespace bg2e {
namespace scene {

class Node;

class BG2E_API BoundingBox : public geo::AABoundingBoxBase
{
public:
    struct Options
    {
        bool includeDisabledNodes = false;
        bool includeInvisibleSubmeshes = false;
    };

    explicit BoundingBox(Node* node);
    BoundingBox(Node* node, const Options& options);

private:
    void processNode(Node* node, const Options& options);
    void addSubmesh(const geo::AABoundingBox<scene::Mesh>& box, const glm::mat4& transform);
    void addPoint(const glm::vec3& point);
};

}
}
