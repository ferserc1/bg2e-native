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

// Current-signal statistics only. V4 is camera-depth-only; no history/output reads.
const float CURRENT_STATISTICS_EPSILON = 1e-6;
struct CurrentStatistics {
    vec3 minimum, maximum;
    float minimumStatistic, maximumStatistic;
    float mean, secondMoment, variance;
    uint sampleCount;
    bool varianceValid;
#if TEMPORAL_MOMENTS
    float momentMean, momentSecondMoment, momentVariance;
#endif
};

CurrentStatistics gatherCurrentStatistics(ivec2 pixel, uint token, vec3 position, vec3 normal)
{
    CurrentStatistics statistics;
    statistics.minimum = vec3(0);
    statistics.maximum = vec3(0);
    statistics.minimumStatistic = 0.0;
    statistics.maximumStatistic = 0.0;
    statistics.mean = 0.0;
    statistics.secondMoment = 0.0;
    statistics.variance = 0.0;
    statistics.sampleCount = 0u;
    statistics.varianceValid = false;
#if TEMPORAL_MOMENTS
    statistics.momentMean = 0.0;
    statistics.momentSecondMoment = 0.0;
    statistics.momentVariance = 0.0;
    float momentSum = 0.0;
    float momentSecondSum = 0.0;
#endif
    float tolerance = dataBlock.rejection.x+dataBlock.rejection.y*length(position-dataBlock.cameraRoughness.xyz);
    if (token == 0u || !finite4(vec4(position,1)) || !finite4(vec4(normal,1)) ||
        !finite4(vec4(tolerance)) || tolerance < 0.0)
    {
        return statistics;
    }
    float sum = 0.0;
    float secondSum = 0.0;
    // Uniform weights; every signal texel appears at most once, including borders.
    for (int index = 0; index < 9; ++index)
    {
        ivec2 neighbor = pixel+ivec2(index%3-1,index/3-1);
        if (any(lessThan(neighbor,ivec2(0))) || any(greaterThanEqual(neighbor,ivec2(dataBlock.extent.zw))))
        {
            continue;
        }
        vec2 neighborUV = (vec2(neighbor)+0.5)/vec2(dataBlock.extent.zw);
        ivec2 receiver = ivec2(floor(neighborUV*vec2(dataBlock.extent.xy)));
        uint neighborToken = texelFetch(usampler2D(tokenImage,nearestSampler),receiver,0).r;
        if (neighborToken == 0u || neighborToken != token)
        {
            continue;
        }
        float neighborDepth = texelFetch(sampler2D(positionImage,nearestSampler),receiver,0).r;
        float metadataDepth = texelFetch(sampler2D(metadataDepthImage,nearestSampler),receiver,0).r;
        vec3 encoded = texelFetch(sampler2D(normalImage,nearestSampler),receiver,0).xyz;
        vec3 decoded = encoded*2.0-1.0;
        float normalLengthSquared = dot(decoded,decoded);
        if (!finite4(vec4(neighborDepth,metadataDepth,normalLengthSquared,1)) ||
            !finite4(vec4(encoded,1)) || neighborDepth < 0.0 || neighborDepth >= 1.0 ||
            metadataDepth < 0.0 || metadataDepth >= 1.0 ||
            abs(metadataDepth-neighborDepth) > dataBlock.metadataRejection.x ||
            dot(encoded,encoded) <= CURRENT_STATISTICS_EPSILON || normalLengthSquared <= CURRENT_STATISTICS_EPSILON)
        {
            continue;
        }
        vec2 receiverUV = (vec2(receiver)+0.5)/vec2(dataBlock.extent.xy);
        vec4 world = dataBlock.inverseViewProjection*vec4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,neighborDepth,1.0);
        if (!finite4(world) || world.w <= CURRENT_STATISTICS_EPSILON)
        {
            continue;
        }
        vec3 neighborPosition = world.xyz/world.w;
        vec3 neighborNormal = decoded/sqrt(max(normalLengthSquared,CURRENT_STATISTICS_EPSILON));
        // World-position consistency is a conservative metric current-depth test.
        if (!finite4(vec4(neighborPosition,1)) || length(neighborPosition-position) > tolerance ||
            dot(neighborNormal,normal) < dataBlock.rejection.z)
        {
            continue;
        }
        vec4 sampleValue = texelFetch(sampler2D(currentImage,nearestSampler),neighbor,0);
        if (!finite4(sampleValue))
        {
            continue;
        }
        vec3 color;
        float statistic;
#if TEMPORAL_SCALAR
        statistic = clamp(sampleValue.r,0.0,1.0);
        color = vec3(statistic);
#else
        if (dataBlock.counts.x == 1u)
        {
            if (sampleValue.a <= 0.5)
            {
                continue;
            }
        }
        else
        {
            float roughness = texelFetch(sampler2D(materialImage,nearestSampler),receiver,0).g;
            // Confidence-zero misses stay valid temporal samples, but give no radiance bounds.
            if (!finite4(vec4(roughness)) || roughness > dataBlock.cameraRoughness.w ||
                clamp(sampleValue.a,0.0,1.0) <= CURRENT_STATISTICS_EPSILON)
            {
                continue;
            }
        }
        color = sampleValue.rgb;
        statistic = max(dot(color,vec3(0.2126,0.7152,0.0722)),0.0);
#endif
        float squared = statistic*statistic;
        if (!finite4(vec4(statistic,squared,0,1)))
        {
            continue;
        }
        if (statistics.sampleCount == 0u)
        {
            statistics.minimum = color;
            statistics.maximum = color;
            statistics.minimumStatistic = statistic;
            statistics.maximumStatistic = statistic;
        }
        else
        {
            statistics.minimum = min(statistics.minimum,color);
            statistics.maximum = max(statistics.maximum,color);
            statistics.minimumStatistic = min(statistics.minimumStatistic,statistic);
            statistics.maximumStatistic = max(statistics.maximumStatistic,statistic);
        }
#if TEMPORAL_MOMENTS
        float momentStatistic = boundedMomentStatistic(statistic);
        momentSum += momentStatistic;
        momentSecondSum += momentStatistic*momentStatistic;
#endif
        sum += statistic;
        secondSum += squared;
        statistics.sampleCount += 1u;
    }
    if (statistics.sampleCount != 0u)
    {
        float denominator = max(float(statistics.sampleCount),CURRENT_STATISTICS_EPSILON);
        statistics.mean = sum/denominator;
        statistics.secondMoment = secondSum/denominator;
        statistics.variance = max(statistics.secondMoment-statistics.mean*statistics.mean,0.0);
        statistics.varianceValid = statistics.sampleCount >= 2u &&
            finite4(vec4(statistics.mean,statistics.secondMoment,statistics.variance,1));
#if TEMPORAL_MOMENTS
        statistics.momentMean = momentSum/denominator;
        statistics.momentSecondMoment = momentSecondSum/denominator;
        statistics.momentVariance = max(statistics.momentSecondMoment-statistics.momentMean*statistics.momentMean,0.0);
        statistics.varianceValid = statistics.varianceValid &&
            finite4(vec4(statistics.momentMean,statistics.momentSecondMoment,statistics.momentVariance,1));
#endif
    }
    return statistics;
}
