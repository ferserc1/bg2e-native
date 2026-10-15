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

#ifndef BG2E_DRAW_COLOR_METAL
#define BG2E_DRAW_COLOR_METAL
#include <metal_stdlib>
using namespace metal;
// Explicit output conversion: do not apply to an sRGB attachment a second time.
inline float3 drawLinearToGamma(float3 color, float gamma)
{
    return pow(max(color, float3(0.0f)), float3(1.0f / max(gamma, 1e-4f)));
}
inline float3 drawGammaToLinear(float3 color, float gamma)
{
    return pow(max(color, float3(0.0f)), float3(max(gamma, 1e-4f)));
}
inline float3 drawColorCorrection(float3 color, float exposure, float gamma, float brightness, float contrast)
{
    float3 mapped = float3(1.0f) - exp(-max(color, float3(0.0f)) * max(exposure, 0.0f));
    return (drawLinearToGamma(mapped, gamma) + brightness) * contrast + (1.0f - contrast) * 0.5f;
}
#endif
