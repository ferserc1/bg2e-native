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

#include <metal_stdlib>
using namespace metal;
struct Input { float4 position [[position]]; uint identifier [[flat]]; };
fragment float4 fragMain(Input input [[stage_in]])
{
    uint id = input.identifier;
    return float4(float(id & 255u), float((id >> 8u) & 255u), float((id >> 16u) & 255u), 255.0) / 255.0;
}
