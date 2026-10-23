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

// All values here use the bounded moment domain, never linear clipping variance.
float momentReactivity(CurrentStatistics statistics, vec2 previousMoments, float effectiveCount, float sigma)
{
    if (!statistics.varianceValid)
    {
        return 0.0;
    }
    float variance = statistics.momentVariance;
    if (effectiveCount >= 4.0)
    {
        float temporalVariance = max(previousMoments.y-previousMoments.x*previousMoments.x,0.0);
        variance = max(variance,temporalVariance);
    }
    float sigmaStd = sigma*sqrt(variance+CURRENT_STATISTICS_EPSILON);
    float delta = abs(previousMoments.x-statistics.momentMean);
    return clamp((delta-sigmaStd)/(delta+sigmaStd+CURRENT_STATISTICS_EPSILON),0.0,1.0);
}
