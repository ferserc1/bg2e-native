//
//  Mesh.hpp
#pragma once

#include <bg2e/geo/Mesh.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>

namespace bg2e {
namespace scene {

// The standard mesh in bg2 engine always have normals, two UV texture sets and tangents
//typedef geo::MeshPNUUT Mesh;
//typedef render::vulkan::geo::MeshPNUUT RenderMesh;

typedef geo::MeshPNUUT Mesh;
typedef render::vulkan::geo::MeshPNUUT RenderMesh;

}
}
