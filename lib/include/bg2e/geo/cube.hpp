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

extern BG2E_API bg2e::geo::MeshP* createCubeP(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPC* createCubePC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPN* createCubePN(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPU* createCubePU(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNU* createCubePNU(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNC* createCubePNC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUC* createCubePNUC(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUT* createCubePNUT(float width, float height, float depth, bool flipFaces = false);
extern BG2E_API bg2e::geo::MeshPNUUT* createCubePNUUT(float width, float height, float depth, bool flipFaces = false);

extern BG2E_API bg2e::geo::MeshPNUUT* createCube(float width, float height, float depth, bool flipFaces = false);

}
}
