/*
 *    business grade graphic engine (bg2e engine)
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

// Visibility [0,1]; HDR log2(1+L) [0,16]. Squared HDR statistic is <=256.
float boundedMomentStatistic(float luminance)
{
#if TEMPORAL_SCALAR
    return clamp(luminance,0.0,1.0);
#else
    return clamp(log2(1.0+max(luminance,0.0)),0.0,16.0);
#endif
}

bool validMoments(vec2 moments)
{
#if TEMPORAL_SCALAR
    vec2 maximum = vec2(1,1);
#else
    vec2 maximum = vec2(16,256);
#endif
    return finite4(vec4(moments,0,1)) && all(greaterThanEqual(moments,vec2(0))) && all(lessThanEqual(moments,maximum));
}
