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

#include <bg2e/geo/Mesh.hpp>
#include <glm/geometric.hpp>

namespace bg2e {
namespace geo {

class AABoundingBoxBase
{
public:
    [[nodiscard]] constexpr const glm::vec3& min() const noexcept { return _min; }
    [[nodiscard]] constexpr const glm::vec3& max() const noexcept { return _max; }
    [[nodiscard]] glm::vec3 size() const noexcept { return _max - _min; }
    [[nodiscard]] glm::vec3 halfSize() const noexcept { return size() * 0.5f; }
    [[nodiscard]] glm::vec3 center() const noexcept { return 0.5f * (_min + _max); }

    bool isValid() const noexcept { return _valid; }

protected:
    AABoundingBoxBase() : _min(0.0f), _max(0.0f), _valid(false) {}
    AABoundingBoxBase(glm::vec3 min, glm::vec3 max, bool valid) : _min(min), _max(max), _valid(valid) {}

    glm::vec3& mutableMin() noexcept { return _min; }
    glm::vec3& mutableMax() noexcept { return _max; }

    bool _valid;

private:
    glm::vec3 _min;
    glm::vec3 _max;
};

template <typename MeshT>
class AABoundingBox : public AABoundingBoxBase
{
public:
    explicit AABoundingBox(const MeshT& mesh)
        : AABoundingBoxBase()
    {
        computeFromVertices(mesh);
    }

    explicit AABoundingBox(const MeshT* mesh)
        : AABoundingBoxBase()
    {
        computeFromVertices(*mesh);
    }

    explicit AABoundingBox(const std::shared_ptr<MeshT> mesh)
        : AABoundingBoxBase()
    {
        computeFromVertices(*mesh);
    }

    explicit AABoundingBox(const MeshT& mesh, uint32_t submeshIndex)
        : AABoundingBoxBase()
    {
        assert(submeshIndex < mesh.submeshes.size());
        if (!(submeshIndex < mesh.submeshes.size())) {
            return;
        }

        computeFromSubmesh(mesh, submeshIndex);
    }

    explicit AABoundingBox(const MeshT* mesh, uint32_t submeshIndex)
        : AABoundingBoxBase()
    {
        assert(submeshIndex < mesh->submeshes.size());
        if (!(submeshIndex < mesh->submeshes.size())) {
            return;
        }

        computeFromSubmesh(*mesh, submeshIndex);
    }

    explicit AABoundingBox(const std::shared_ptr<MeshT> mesh, uint32_t submeshIndex)
        : AABoundingBoxBase()
    {
        assert(submeshIndex < mesh->submeshes.size());
        if (!(submeshIndex < mesh->submeshes.size())) {
            return;
        }

        computeFromSubmesh(*mesh, submeshIndex);
    }

private:
    void computeFromVertices(const MeshT& mesh) {
        assert(!mesh.vertices.empty());
        if (mesh.vertices.empty()) {
            return;
        }

        glm::vec3 bbMin = mesh.vertices[0].position;
        glm::vec3 bbMax = bbMin;

        for (const auto& v : mesh.vertices) {
            bbMin = glm::min(bbMin, v.position);
            bbMax = glm::max(bbMax, v.position);
        }

        mutableMin() = bbMin;
        mutableMax() = bbMax;
        _valid = true;
    }

    void computeFromSubmesh(const MeshT& mesh, uint32_t submeshIndex) {
        const auto& submesh = mesh.submeshes[submeshIndex];

        assert(!mesh.vertices.empty());
        if (mesh.vertices.empty()) {
            return;
        }

        glm::vec3 bbMin = mesh.vertices[0].position;
        glm::vec3 bbMax = bbMin;

        const auto* indices = mesh.indices.data();
        for (uint32_t i = 0; i < submesh.indexCount; ++i) {
            uint32_t idx = indices[submesh.firstIndex + i];
            bbMin = glm::min(bbMin, mesh.vertices[idx].position);
            bbMax = glm::max(bbMax, mesh.vertices[idx].position);
        }

        mutableMin() = bbMin;
        mutableMax() = bbMax;
        _valid = true;
    }
};

}
}
