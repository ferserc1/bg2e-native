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
