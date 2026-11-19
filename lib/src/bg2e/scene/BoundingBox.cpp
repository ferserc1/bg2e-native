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

#include <bg2e/scene/BoundingBox.hpp>

#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>

#include <array>
#include <algorithm>
#include <bg2e/math/glm.hpp>

namespace bg2e::scene {

BoundingBox::BoundingBox(Node* node)
    : BoundingBox(node, Options{})
{
}

BoundingBox::BoundingBox(Node* node, const Options& options)
{
    processNode(node, options);
}

void BoundingBox::processNode(Node* node, const Options& options)
{
    if (!node || (node->disabled() && !options.includeDisabledNodes))
    {
        return;
    }

    if (const auto* component = node->drawableRender())
    {
        const auto mesh = component->geometry();
        if (mesh)
        {
            const auto world = node->worldMatrix();
            if (mesh->submeshes.empty())
            {
                geo::AABoundingBox<geo::MeshPNUUT> box(*mesh);
                addSubmesh(box, world * component->transform());
            }
            else
            {
                const auto count = std::min(component->submeshesCount(), static_cast<uint32_t>(mesh->submeshes.size()));
                for (uint32_t i = 0; i < count; ++i)
                {
                    if (!options.includeInvisibleSubmeshes &&
                        (!component->submeshVisibility(i) || !component->effectiveMaterial(i).visible()))
                    {
                        continue;
                    }
                    geo::AABoundingBox<geo::MeshPNUUT> box(*mesh, i);
                    addSubmesh(box, world * component->submeshTransform(i));
                }
            }
        }
    }

    for (const auto& child : node->children())
    {
        processNode(child.get(), options);
    }
}

void BoundingBox::addSubmesh(const geo::AABoundingBox<geo::MeshPNUUT>& box, const glm::mat4& transform)
{
    if (!box.isValid())
    {
        return;
    }

    const auto& min = box.min();
    const auto& max = box.max();
    const std::array<glm::vec3, 8> corners = {
        glm::vec3{min.x, min.y, min.z},
        glm::vec3{min.x, min.y, max.z},
        glm::vec3{min.x, max.y, min.z},
        glm::vec3{min.x, max.y, max.z},
        glm::vec3{max.x, min.y, min.z},
        glm::vec3{max.x, min.y, max.z},
        glm::vec3{max.x, max.y, min.z},
        glm::vec3{max.x, max.y, max.z}};

    for (const auto& corner : corners)
    {
        addPoint(glm::vec3(transform * glm::vec4(corner, 1.0f)));
    }
}

void BoundingBox::addPoint(const glm::vec3& point)
{
    if (!_valid)
    {
        mutableMin() = point;
        mutableMax() = point;
        _valid = true;
        return;
    }

    mutableMin() = glm::min(mutableMin(), point);
    mutableMax() = glm::max(mutableMax(), point);
}

}
