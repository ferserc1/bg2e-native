#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

extern BG2E_API bg2e::geo::MeshP* createSpherP(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPC* createSpherePC(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPN* createSpherePN(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPU* createSpherePU(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNU* createSpherePNU(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNC* createSpherePNC(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNUC* createSpherePNUC(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNUT* createSpherePNUT(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createSpherePNUUT(float radius, uint32_t longitudes, uint32_t latitudes, bool invertFaces = false);

// Default sphere type is PNUUT
extern BG2E_API bg2e::geo::MeshPNUUT* createSphere(float radius, uint32_t longitudes, uint32_t latitudes);

}
}
