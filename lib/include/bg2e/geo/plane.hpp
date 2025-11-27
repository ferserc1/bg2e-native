#pragma once

#include <bg2e/common.hpp>

#include <bg2e/geo/Mesh.hpp>

namespace bg2e {
namespace geo {

extern BG2E_API bg2e::geo::MeshP* createPlaneP(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPC* createPlanePC(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPN* createPlanePN(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPU* createPlanePU(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPNU* createPlanePNU(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPNC* createPlanePNC(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPNUC* createPlanePNUC(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPNUT* createPlanePNUT(float width, float depth, bool invertFace = false);
extern BG2E_API bg2e::geo::MeshPNUUT* createPlanePNUUT(float width, float depth, bool invertFace = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createPlane(float width, float depth, bool invertFace = false);

}
}
