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

// Local response only; original history storage and alpha/confidence are untouched.
struct HistoryResponse {
    vec4 history;
    float reactivity;
};

HistoryResponse respondToCurrentStatistics(vec4 history, CurrentStatistics statistics, float sigma)
{
    HistoryResponse response;
    response.history = history;
    response.reactivity = 0.0;
    if (!statistics.varianceValid)
    {
        return response;
    }
    float sigmaStd = sigma*sqrt(statistics.variance+CURRENT_STATISTICS_EPSILON);
    float lower = max(statistics.minimumStatistic,statistics.mean-sigmaStd);
    float upper = min(statistics.maximumStatistic,statistics.mean+sigmaStd);
    // Roundoff can make a sigma-zero intersection empty; retain the baseline then.
    if (!finite4(vec4(sigmaStd,lower,upper,1)) || lower > upper)
    {
        return response;
    }
#if TEMPORAL_SCALAR
    float historicalStatistic = history.r;
    response.history.r = clamp(historicalStatistic,lower,upper);
#else
    float historicalStatistic = max(dot(history.rgb,vec3(0.2126,0.7152,0.0722)),0.0);
    if (historicalStatistic > CURRENT_STATISTICS_EPSILON)
    {
        float clippedStatistic = clamp(historicalStatistic,lower,upper);
        vec3 scaled = history.rgb*(clippedStatistic/historicalStatistic);
        // Output histories are FP16: hue-preserving scaling may exceed one channel's range.
        response.history.rgb = clamp(scaled,vec3(-65504.0),vec3(65504.0));
    }
    else
    {
        response.history.rgb = clamp(history.rgb,statistics.minimum,statistics.maximum);
    }
#endif
    float delta = abs(historicalStatistic-statistics.mean);
    response.reactivity = clamp((delta-sigmaStd)/(delta+sigmaStd+CURRENT_STATISTICS_EPSILON),0.0,1.0);
    if (!finite4(response.history) || !finite4(vec4(response.reactivity)))
    {
        response.history = history;
        response.reactivity = 0.0;
    }
    return response;
}
