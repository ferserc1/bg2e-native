#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

extern BG2E_API bg2e::geo::MeshP* createCylinderP(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPC* createCylinderPC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPN* createCylinderPN(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPU* createCylinderPU(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNU* createCylinderPNU(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNC* createCylinderPNC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUC* createCylinderPNUC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUT* createCylinderPNUT(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUUT* createCylinderPNUUT(float radius, float height, uint32_t slices, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createCylinder(float radius, float height, uint32_t slices, bool invertFaces = false);

}
}
