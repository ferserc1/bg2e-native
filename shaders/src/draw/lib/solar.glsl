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

#ifndef BG2E_SOLAR
#define BG2E_SOLAR

float solarRadiusRatio(float size, float distanceMillionKm)
{
    const float SOLAR_RADIUS_KM = 695700.0;
    return clamp(size * (SOLAR_RADIUS_KM / 1e6) / max(distanceMillionKm, 1e-6), 1e-12, 0.999999);
}

float solarAngularRadius(float size, float distanceMillionKm)
{
    return asin(solarRadiusRatio(size, distanceMillionKm));
}

float solarDiskNormalization(float size, float distanceMillionKm)
{
    float ratio = solarRadiusRatio(size, distanceMillionKm);
    return 1.0 / (3.141592653589793 * ratio * ratio);
}

float solarVisibility(float elevationY, float size, float distanceMillionKm)
{
    float fraction = clamp(asin(clamp(elevationY, -1.0, 1.0)) / solarAngularRadius(size, distanceMillionKm), -1.0, 1.0);
    return (acos(-fraction) + fraction * sqrt(max(1.0 - fraction * fraction, 0.0))) / 3.141592653589793;
}

#endif
