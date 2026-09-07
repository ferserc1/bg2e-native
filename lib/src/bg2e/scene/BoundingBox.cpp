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

#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>

#include <array>
#include <algorithm>
#include <glm/common.hpp>

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

    auto drawableComponent = node->drawable();
    auto drawable = drawableComponent ? drawableComponent->drawable() : nullptr;
    auto mesh = drawable ? drawable->mesh() : nullptr;

    if (mesh)
    {
        const auto worldMatrix = node->worldMatrix();
        const auto submeshCount = drawable->submeshesCount();

        if (mesh->submeshes.empty())
        {
            geo::AABoundingBox<scene::Mesh> box(*mesh);
            addSubmesh(box, worldMatrix);
        }
        else
        {
            const auto count = std::min(
                submeshCount,
                static_cast<uint32_t>(mesh->submeshes.size())
            );

            for (uint32_t submeshIndex = 0; submeshIndex < count; ++submeshIndex)
            {
                if (!options.includeInvisibleSubmeshes && !drawable->submeshVisibility(submeshIndex))
                {
                    continue;
                }

                geo::AABoundingBox<scene::Mesh> box(*mesh, submeshIndex);
                auto transform = worldMatrix * drawable->localSubmeshTransform(submeshIndex);
                addSubmesh(box, transform);
            }
        }
    }

    for (const auto& child : node->children())
    {
        processNode(child.get(), options);
    }
}

void BoundingBox::addSubmesh(const geo::AABoundingBox<scene::Mesh>& box, const glm::mat4& transform)
{
    if (!box.isValid())
    {
        return;
    }

    const auto& min = box.min();
    const auto& max = box.max();
    const std::array<glm::vec3, 8> corners = {
        glm::vec3{ min.x, min.y, min.z },
        glm::vec3{ min.x, min.y, max.z },
        glm::vec3{ min.x, max.y, min.z },
        glm::vec3{ min.x, max.y, max.z },
        glm::vec3{ max.x, min.y, min.z },
        glm::vec3{ max.x, min.y, max.z },
        glm::vec3{ max.x, max.y, min.z },
        glm::vec3{ max.x, max.y, max.z }
    };

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
