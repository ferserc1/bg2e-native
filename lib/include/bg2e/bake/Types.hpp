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
#include <cstdint>

namespace bg2e::bake {
enum class LightmapMode : uint32_t
{
    RTAO, RTGI
};
enum class LightmapPixelFormat : uint32_t
{
    RGB8, RGB32F
};
enum class BakeState : uint32_t
{
    Ready, Running, Complete, Cancelled, Invalidated, Failed
};
// Presence is an explicit result/binding property, never inferred from material AO.
enum class LightmapSignal : uint32_t
{
    IndirectMultiplierRGB
};
}
