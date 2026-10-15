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

#ifndef BG2E_DRAW_COLOR_GLSL
#define BG2E_DRAW_COLOR_GLSL
// Explicit output conversion: do not apply to an sRGB attachment a second time.
vec3 drawLinearToGamma(vec3 color, float gamma)
{
    return pow(max(color, vec3(0.0)), vec3(1.0 / max(gamma, 1e-4)));
}
vec3 drawGammaToLinear(vec3 color, float gamma)
{
    return pow(max(color, vec3(0.0)), vec3(max(gamma, 1e-4)));
}
vec3 drawColorCorrection(vec3 color, float exposure, float gamma, float brightness, float contrast)
{
    vec3 mapped = vec3(1.0) - exp(-max(color, vec3(0.0)) * max(exposure, 0.0));
    return (drawLinearToGamma(mapped, gamma) + brightness) * contrast + (1.0 - contrast) * 0.5;
}
#endif
