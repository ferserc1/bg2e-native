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
