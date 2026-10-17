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

#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/pipeline/RTGlobalIllumination.hpp>
#include <bg2e/draw/pipeline/RTReflections.hpp>
#include <bg2e/draw/pipeline/TemporalAccumulator.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <limits>

namespace bg2e::draw::pipeline {

reflection::TypeInfo RTAmbientOcclusion::settingsType() const
{
    using Effect = RTAmbientOcclusion;
    reflection::TypeInfoBuilder<Effect> type("bg2e.draw.rtao.v1");
    type.property("enabled", &Effect::enabled, &Effect::setEnabled).category("Effect");
    type.property("quality", &Effect::quality, &Effect::setQuality).category("Effect").enumValue("Low",0).enumValue("Medium",1).enumValue("High",2).enumValue("Ultra",3);
    type.property("sampleCount", &Effect::sampleCount, &Effect::setSampleCount).category("Effect").range(1,64);
    type.property("bias", &Effect::bias, &Effect::setBias).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("useBlueNoise", &Effect::useBlueNoise, &Effect::setUseBlueNoise).category("Effect");
    type.property("bounceCount", &Effect::bounceCount, &Effect::setBounceCount).category("Effect").range(1,8);
    type.property("radius", &Effect::radius, &Effect::setRadius).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("falloff", &Effect::falloff, &Effect::setFalloff).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("bounceAttenuation", &Effect::bounceAttenuation, &Effect::setBounceAttenuation).category("Effect").range(0,1);
    type.property("available", &Effect::available).category("Capabilities");
    type.property("status", &Effect::status).category("Capabilities");
    type.property("tableCapacity", &Effect::tableCapacity).category("Capabilities");
    type.accessor<bool>("blueNoiseActive", [](const Effect& o)
    {
        return o.available() && o.useBlueNoise() && o.noise() && o.noise()->isReady();
    }).category("Capabilities");
    return type.build();
}

void* RTAmbientOcclusion::settingsInstance()
{
    return this;
}

reflection::TypeInfo RTGlobalIllumination::settingsType() const
{
    using Effect = RTGlobalIllumination;
    reflection::TypeInfoBuilder<Effect> type("bg2e.draw.rtgi.v1");
    type.property("enabled", &Effect::enabled, &Effect::setEnabled).category("Effect");
    type.property("quality", &Effect::quality, &Effect::setQuality).category("Effect").enumValue("Low",0).enumValue("Medium",1).enumValue("High",2).enumValue("Ultra",3);
    type.property("sampleCount", &Effect::sampleCount, &Effect::setSampleCount).category("Effect").range(1,64);
    type.property("bias", &Effect::bias, &Effect::setBias).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("useBlueNoise", &Effect::useBlueNoise, &Effect::setUseBlueNoise).category("Effect");
    type.property("bounceCount", &Effect::bounceCount, &Effect::setBounceCount).category("Effect").range(1,8);
    type.property("maxDistance", &Effect::maxDistance, &Effect::setMaxDistance).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("shadowSamples", &Effect::shadowSamples, &Effect::setShadowSamples).category("Effect").range(1,64);
    type.property("useShadows", &Effect::useShadows, &Effect::setUseShadows).category("Effect");
    type.property("available", &Effect::available).category("Capabilities");
    type.property("status", &Effect::status).category("Capabilities");
    type.property("tableCapacity", &Effect::tableCapacity).category("Capabilities");
    type.accessor<bool>("blueNoiseActive", [](const Effect& o)
    {
        return o.available() && o.useBlueNoise() && o.noise() && o.noise()->isReady();
    }).category("Capabilities");
    return type.build();
}

void* RTGlobalIllumination::settingsInstance()
{
    return this;
}

reflection::TypeInfo RTReflections::settingsType() const
{
    using Effect = RTReflections;
    reflection::TypeInfoBuilder<Effect> type("bg2e.draw.reflections.v1");
    type.property("enabled", &Effect::enabled, &Effect::setEnabled).category("Effect");
    type.property("quality", &Effect::quality, &Effect::setQuality).category("Effect").enumValue("Low",0).enumValue("Medium",1).enumValue("High",2).enumValue("Ultra",3);
    type.property("sampleCount", &Effect::sampleCount, &Effect::setSampleCount).category("Effect").range(1,64);
    type.property("bias", &Effect::bias, &Effect::setBias).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("useBlueNoise", &Effect::useBlueNoise, &Effect::setUseBlueNoise).category("Effect");
    type.property("maxDistance", &Effect::maxDistance, &Effect::setMaxDistance).category("Effect").range(std::numeric_limits<float>::denorm_min(),std::numeric_limits<float>::max());
    type.property("shadowSamples", &Effect::shadowSamples, &Effect::setShadowSamples).category("Effect").range(1,64);
    type.property("useShadows", &Effect::useShadows, &Effect::setUseShadows).category("Effect");
    type.property("maxRoughness", &Effect::maxRoughness, &Effect::setMaxRoughness).category("Effect").range(0,1);
    type.property("roughnessSpread", &Effect::roughnessSpread, &Effect::setRoughnessSpread).category("Effect").range(0,16);
    type.property("available", &Effect::available).category("Capabilities");
    type.property("status", &Effect::status).category("Capabilities");
    type.property("tableCapacity", &Effect::tableCapacity).category("Capabilities");
    type.accessor<bool>("blueNoiseActive", [](const Effect& o)
    {
        return o.available() && o.useBlueNoise() && o.noise() && o.noise()->isReady();
    }).category("Capabilities");
    return type.build();
}

void* RTReflections::settingsInstance()
{
    return this;
}

reflection::TypeInfo TemporalAccumulator::settingsType() const
{
    using Effect = TemporalAccumulator;
    reflection::TypeInfoBuilder<Effect> type("bg2e.draw.temporal.v1");
    type.accessor<bool>("enabled", [](const Effect& o)
    {
        return o.settings().enabled;
    }, [](Effect& o, bool v)
    {
        o.setEnabled(v);
    }).category("Temporal");
    type.accessor<bool>("freezeAtLimit", [](const Effect& o)
    {
        return o.settings().freezeAtLimit;
    }, [](Effect& o, bool v)
    {
        o.setFreezeAtLimit(v);
    }).category("Temporal");
    type.accessor<uint32_t>("maxFrames", [](const Effect& o)
    {
        return o.settings().maxFrames;
    }, [](Effect& o, uint32_t v)
    {
        o.setMaxFrames(v);
    }).category("Temporal").range(1,1024);
    type.accessor<float>("absolutePositionThreshold", [](const Effect& o)
    {
        return o.settings().absolutePositionThreshold;
    }, [](Effect& o, float v)
    {
        o.setPositionThresholds(v,o.settings().relativePositionThreshold);
    }).category("Temporal").range(0,std::numeric_limits<float>::max());
    type.accessor<float>("relativePositionThreshold", [](const Effect& o)
    {
        return o.settings().relativePositionThreshold;
    }, [](Effect& o, float v)
    {
        o.setPositionThresholds(o.settings().absolutePositionThreshold,v);
    }).category("Temporal").range(0,std::numeric_limits<float>::max());
    type.accessor<float>("normalThreshold", [](const Effect& o)
    {
        return o.settings().normalThreshold;
    }, [](Effect& o, float v)
    {
        o.setNormalThreshold(v);
    }).category("Temporal").range(-1,1);
    type.property("available", &Effect::available).category("Capabilities");
    type.property("status", &Effect::status).category("Capabilities");
    return type.build();
}

void* TemporalAccumulator::settingsInstance()
{
    return this;
}
}
