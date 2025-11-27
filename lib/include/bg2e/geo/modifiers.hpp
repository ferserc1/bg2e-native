#pragma once

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

class Modifier {
public:
    virtual ~Modifier() {}
    
    virtual void apply() = 0;
};

template <class MeshT>
class FlipFacesModifier : public Modifier {
public:
    FlipFacesModifier(MeshT * mesh)
        :_mesh { mesh } {}
    
    void apply() override
    {
        for (uint32_t i = 0; i < uint32_t(_mesh->indices.size()); i += 3)
        {
            auto i0 = _mesh->indices[i];
            auto i2 = _mesh->indices[i + 2];
            
            _mesh->indices[i] = i2;
            _mesh->indices[i + 2] = i0;
        }
    }

protected:
    MeshT * _mesh;
};

// Obviously, this modifier only works with meshes that contains tangent coords (MeshPNUT and MeshPNUUT).
template <class MeshT>
class GenTangentsModifier : public Modifier {
public:
    GenTangentsModifier(MeshT * mesh)
        :_mesh { mesh } {}
        
    void apply() override
    {        
        for (size_t i = 0; i < _mesh->indices.size(); i+=3)
        {
            auto i1 = _mesh->indices[i];
            auto i2 = _mesh->indices[i + 1];
            auto i3 = _mesh->indices[i + 2];
            
            auto& v0 = _mesh->vertices[i1];
            auto& v1 = _mesh->vertices[i2];
            auto& v2 = _mesh->vertices[i3];
            
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
            
            glm::vec3 t {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
            };
            v0.tangent = t;
            v1.tangent = t;
            v2.tangent = t;
        }
    }

protected:
    MeshT * _mesh;
};


}
}
