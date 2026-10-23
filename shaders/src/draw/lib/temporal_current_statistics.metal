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
constant float CURRENT_STATISTICS_EPSILON = 1e-6;
struct CurrentStatistics {
    float3 minimum, maximum;
    float minimumStatistic, maximumStatistic;
    float mean, secondMoment, variance;
    uint sampleCount;
    bool varianceValid;
#if TEMPORAL_MOMENTS
    float momentMean, momentSecondMoment, momentVariance;
#endif
};

CurrentStatistics gatherCurrentStatistics(int2 pixel, uint token, float3 position, float3 normal,
    constant TemporalUniforms& dataBlock, texture2d<float> normalImage, depth2d<float> positionImage,
    texture2d<float> currentImage, texture2d<float> materialImage, texture2d<uint> tokenImage,
    depth2d<float> metadataDepthImage)
{
    CurrentStatistics statistics;
    statistics.minimum = float3(0);
    statistics.maximum = float3(0);
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
    if (token == 0u || !finite4(float4(position,1)) || !finite4(float4(normal,1)) ||
        !finite4(float4(tolerance)) || tolerance < 0.0)
    {
        return statistics;
    }
    float sum = 0.0;
    float secondSum = 0.0;
    // Uniform weights; every signal texel appears at most once, including borders.
    for (int index = 0; index < 9; ++index)
    {
        int2 neighbor = pixel+int2(index%3-1,index/3-1);
        if (any(neighbor < int2(0)) || any(neighbor >= int2(dataBlock.extent.zw)))
        {
            continue;
        }
        float2 neighborUV = (float2(neighbor)+0.5)/float2(dataBlock.extent.zw);
        int2 receiver = int2(floor(neighborUV*float2(dataBlock.extent.xy)));
        uint neighborToken = tokenImage.read(uint2(receiver)).r;
        if (neighborToken == 0u || neighborToken != token)
        {
            continue;
        }
        float neighborDepth = positionImage.read(uint2(receiver));
        float metadataDepth = metadataDepthImage.read(uint2(receiver));
        float3 encoded = normalImage.read(uint2(receiver)).xyz;
        float3 decoded = encoded*2.0-1.0;
        float normalLengthSquared = dot(decoded,decoded);
        if (!finite4(float4(neighborDepth,metadataDepth,normalLengthSquared,1)) ||
            !finite4(float4(encoded,1)) || neighborDepth < 0.0 || neighborDepth >= 1.0 ||
            metadataDepth < 0.0 || metadataDepth >= 1.0 ||
            abs(metadataDepth-neighborDepth) > dataBlock.metadataRejection.x ||
            dot(encoded,encoded) <= CURRENT_STATISTICS_EPSILON || normalLengthSquared <= CURRENT_STATISTICS_EPSILON)
        {
            continue;
        }
        float2 receiverUV = (float2(receiver)+0.5)/float2(dataBlock.extent.xy);
        float4 world = dataBlock.inverseViewProjection*float4(receiverUV.x*2.0-1.0,1.0-receiverUV.y*2.0,neighborDepth,1.0);
        if (!finite4(world) || world.w <= CURRENT_STATISTICS_EPSILON)
        {
            continue;
        }
        float3 neighborPosition = world.xyz/world.w;
        float3 neighborNormal = decoded/sqrt(max(normalLengthSquared,CURRENT_STATISTICS_EPSILON));
        // World-position consistency is a conservative metric current-depth test.
        if (!finite4(float4(neighborPosition,1)) || length(neighborPosition-position) > tolerance ||
            dot(neighborNormal,normal) < dataBlock.rejection.z)
        {
            continue;
        }
        float4 sampleValue = currentImage.read(uint2(neighbor));
        if (!finite4(sampleValue))
        {
            continue;
        }
        float3 color;
        float statistic;
#if TEMPORAL_SCALAR
        statistic = clamp(sampleValue.r,0.0,1.0);
        color = float3(statistic);
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
            float roughness = materialImage.read(uint2(receiver)).g;
            // Confidence-zero misses stay valid temporal samples, but give no radiance bounds.
            if (!finite4(float4(roughness)) || roughness > dataBlock.cameraRoughness.w ||
                clamp(sampleValue.a,0.0,1.0) <= CURRENT_STATISTICS_EPSILON)
            {
                continue;
            }
        }
        color = sampleValue.rgb;
        statistic = max(dot(color,float3(0.2126,0.7152,0.0722)),0.0);
#endif
        float squared = statistic*statistic;
        if (!finite4(float4(statistic,squared,0,1)))
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
            finite4(float4(statistics.mean,statistics.secondMoment,statistics.variance,1));
#if TEMPORAL_MOMENTS
        statistics.momentMean = momentSum/denominator;
        statistics.momentSecondMoment = momentSecondSum/denominator;
        statistics.momentVariance = max(statistics.momentSecondMoment-statistics.momentMean*statistics.momentMean,0.0);
        statistics.varianceValid = statistics.varianceValid &&
            finite4(float4(statistics.momentMean,statistics.momentSecondMoment,statistics.momentVariance,1));
#endif
    }
    return statistics;
}
