#pragma once

#include <bg2e/common.hpp>
#include <bg2e/geo/Vertex.hpp>

#include <vector>

namespace bg2e {
namespace geo {

struct Submesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
};

template <typename VertexT>
struct MeshGeneric {
    std::vector<VertexT> vertices;
    std::vector<uint32_t> indices;
    std::vector<Submesh> submeshes;
};

typedef MeshGeneric<VertexP> MeshP;
typedef MeshGeneric<VertexPN> MeshPN;
typedef MeshGeneric<VertexPC> MeshPC;
typedef MeshGeneric<VertexPU> MeshPU;
typedef MeshGeneric<VertexPNU> MeshPNU;
typedef MeshGeneric<VertexPNC> MeshPNC;
typedef MeshGeneric<VertexPNUC> MeshPNUC;
typedef MeshGeneric<VertexPNUT> MeshPNUT;
typedef MeshGeneric<VertexPNUUT> MeshPNUUT;

// Default mesh type
typedef MeshGeneric<Vertex> Mesh;

}
}
