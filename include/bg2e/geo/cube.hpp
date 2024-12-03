#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

extern BG2E_API bg2e::geo::MeshP* createCubeP(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPC* createCubePC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPN* createCubePN(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPU* createCubePU(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNU* createCubePNU(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNC* createCubePNC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUC* createCubePNUC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUT* createCubePNUT(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUUT* createCubePNUUT(float width, float height, float depth, bool flipFaces = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createCube(float width, float height, float depth, float flipFaces = false);

}
}
