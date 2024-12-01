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

}
}
