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

namespace bg2e {
namespace geo {

template <class MeshT>
class Modifier {
public:
    Modifier() : _mesh { nullptr } {}
    Modifier(MeshT * mesh) : _mesh { mesh } {}
    virtual ~Modifier() {}

    inline void setMesh(MeshT* mesh) { _mesh = mesh; }
    inline MeshT* mesh() const { return _mesh; }

    virtual void apply() = 0;

protected:
    MeshT * _mesh = nullptr;
};

template <class MeshT>
class FlipFacesModifier : public Modifier<MeshT> {
public:
    FlipFacesModifier()
        :Modifier<MeshT>()
    {}

    FlipFacesModifier(MeshT * mesh)
        :Modifier<MeshT>(mesh)
    {}
    
    void apply() override
    {
        for (uint32_t i = 0; i < uint32_t(this->_mesh->indices.size()); i += 3)
        {
            auto i0 = this->_mesh->indices[i];
            auto i2 = this->_mesh->indices[i + 2];
            
            this->_mesh->indices[i] = i2;
            this->_mesh->indices[i + 2] = i0;
        }
    }
};

// Obviously, this modifier only works with meshes that contains tangent coords (MeshPNUT and MeshPNUUT).
template <class MeshT>
class GenTangentsModifier : public Modifier<MeshT> {
public:
    GenTangentsModifier()
        :Modifier<MeshT>()
    {}

    GenTangentsModifier(MeshT * mesh)
        :Modifier<MeshT>(mesh)
    {}
        
    void apply() override
    {        
        for (size_t i = 0; i < this->_mesh->indices.size(); i+=3)
        {
            auto i1 = this->_mesh->indices[i];
            auto i2 = this->_mesh->indices[i + 1];
            auto i3 = this->_mesh->indices[i + 2];
            
            auto& v0 = this->_mesh->vertices[i1];
            auto& v1 = this->_mesh->vertices[i2];
            auto& v2 = this->_mesh->vertices[i3];
            
            auto pos1 = v0.position;
            auto pos2 = v1.position;
            auto pos3 = v2.position;
            
            auto uv1 = v0.texCoord0;
            auto uv2 = v1.texCoord0;
            auto uv3 = v2.texCoord0;
            
            auto edge1 = pos2 - pos1;
            auto edge2 = pos3 - pos2;
            auto deltaUV1 = uv2 - uv1;
            auto deltaUV2 = uv3 - uv1;
            
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            
            glm::vec3 t = glm::normalize(glm::vec3{
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
            });

            v0.tangent = t;
            v1.tangent = t;
            v2.tangent = t;
        }
    }
};

// This modifier requires meshes that contains positions, normals and tangents
template <class MeshT>
class ApplyTransformModifier : public Modifier<MeshT>
{
public:
    ApplyTransformModifier(const glm::mat4 & transform)
        :Modifier<MeshT>()
        ,_transform(transform)
    {}

    ApplyTransformModifier(MeshT * mesh, const glm::mat4 & transform)
        :Modifier<MeshT>(mesh)
        ,_transform { transform }
    {}

    void apply() override
    {
        auto RM = glm::mat3(_transform);
        auto NM = glm::transpose(glm::inverse(RM));
        for (auto & v : this->_mesh->vertices)
        {
            v.position = glm::vec3( _transform * glm::vec4(v.position, 1.0f) );
            v.normal = glm::normalize(RM * v.normal);
            v.tangent = glm::normalize(RM * v.tangent);
        }
    }

protected:
    glm::mat4 _transform;
};

// TODO: Code not tested
template <class MeshT>
class CenterGeometryModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    void apply() override {
        if (!_mesh || _mesh->vertices.empty()) return;

        glm::vec3 min = _mesh->vertices[0].position;
        glm::vec3 max = min;

        // Compute bounding box
        for (auto &v : _mesh->vertices) {
            min = glm::min(min, v.position);
            max = glm::max(max, v.position);
        }

        glm::vec3 center = 0.5f * (min + max);

        // Apply translation
        for (auto &v : _mesh->vertices) {
            v.position -= center;
        }
    }
};

// TODO: Code not tested
// Scale the mesh so that its size is no larger than the specified size.
template <class MeshT>
class NormalizeScaleModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    NormalizeScaleModifier(float target = 1.0f) : _target(target) {}

    void apply() override {
        if (!_mesh || _mesh->vertices.empty()) return;

        glm::vec3 min = _mesh->vertices[0].position;
        glm::vec3 max = min;

        for (auto &v : _mesh->vertices) {
            min = glm::min(min, v.position);
            max = glm::max(max, v.position);
        }

        glm::vec3 size = max - min;
        float scale = _target / glm::compMax(glm::abs(size));

        for (auto &v : _mesh->vertices) {
            v.position *= scale;
        }
    }

private:
    float _target;
};

// Swap the Y-up and Z-up axes
template <class MeshT>
class ConvertAxisModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    enum class Mode {
        YtoZ,
        ZtoY
    };

    ConvertAxisModifier(Mode mode)
        : _mode(mode)
    {}

    void apply() override {
        if (!_mesh) return;

        for (auto &v : _mesh->vertices) {
            glm::vec3 p = v.position;
            glm::vec3 n = v.normal;
            glm::vec3 t = v.tangent;

            if (_mode == Mode::YtoZ) {
                v.position = { p.x, p.z, -p.y };
                v.normal   = glm::normalize(glm::vec3{ n.x, n.z, -n.y });
                v.tangent  = glm::normalize(glm::vec3{ t.x, t.z, -t.y });
            }
            else { // ZtoY
                v.position = { p.x, -p.z, p.y };
                v.normal   = glm::normalize(glm::vec3{ n.x, -n.z, n.y });
                v.tangent  = glm::normalize(glm::vec3{ t.x, -t.z, t.y });
            }
        }
    }

private:
    Mode _mode;
};

// TODO: Code not tested
// Recalculate the normals per triangle and average them.
template <class MeshT>
class RecalculateNormalsModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    void apply() override {
        if (!_mesh) return;

        // Clear normals
        for (auto &v : _mesh->vertices) {
            v.normal = glm::vec3(0.0f);
        }

        // Accumulate triangle normals
        for (size_t i = 0; i < _mesh->indices.size(); i += 3) {
            uint32_t i0 = _mesh->indices[i];
            uint32_t i1 = _mesh->indices[i + 1];
            uint32_t i2 = _mesh->indices[i + 2];

            auto& v0 = _mesh->vertices[i0];
            auto& v1 = _mesh->vertices[i1];
            auto& v2 = _mesh->vertices[i2];

            glm::vec3 n = glm::normalize(glm::cross(
                v1.position - v0.position,
                v2.position - v0.position
            ));

            v0.normal += n;
            v1.normal += n;
            v2.normal += n;
        }

        // Normalize final normals
        for (auto &v : _mesh->vertices) {
            v.normal = glm::normalize(v.normal);
        }
    }
};


// TODO: Code not tested
// Move the geometry so that the new pivot is the origin.
template <class MeshT>
class BakePivotModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    BakePivotModifier(glm::vec3 pivot)
        : _pivot(pivot)
    {}

    void apply() override {
        if (!_mesh) return;

        for (auto &v : _mesh->vertices) {
            v.position -= _pivot;
        }
    }

private:
    glm::vec3 _pivot;
};


// TODO: Code not tested
// Reverse the geometry on one or more axes
template <class MeshT>
class MirrorModifier : public Modifier<MeshT> {
public:
    using Modifier<MeshT>::_mesh;

    MirrorModifier(bool mx, bool my, bool mz)
        : _mx(mx), _my(my), _mz(mz)
    {}

    void apply() override {
        if (!_mesh) return;

        glm::vec3 scale(
            _mx ? -1.0f : 1.0f,
            _my ? -1.0f : 1.0f,
            _mz ? -1.0f : 1.0f
        );

        for (auto &v : _mesh->vertices) {
            v.position *= scale;
            v.normal   = glm::normalize(v.normal * scale);
            v.tangent  = glm::normalize(v.tangent * scale);
        }
    }

private:
    bool _mx, _my, _mz;
};

}
}
