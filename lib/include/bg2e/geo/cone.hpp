#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

extern BG2E_API bg2e::geo::MeshP* createConeP(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPC* createConePC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPN* createConePN(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPU* createConePU(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNU* createConePNU(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNC* createConePNC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUC* createConePNUC(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUT* createConePNUT(float radius, float height, uint32_t slices, bool invertFaces = false);
extern BG2E_API bg2e::geo::MeshPNUUT* createConePNUUT(float radius, float height, uint32_t slices, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createCone(float radius, float height, uint32_t slices, bool invertFaces = false);

}
}
