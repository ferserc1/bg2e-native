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

#include <bg2e/bake/LightmapSettings.hpp>
#include <bg2e/reflection/Builder.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::bake {
void LightmapSettings::validate() const
{
    const float distance = mode() == LightmapMode::RTAO ? aoRadius() : maxRayDistance();
    if (distance <= 2*rayBias())
    {
        throw std::invalid_argument("Bake ray distance/radius must exceed twice the ray bias");
    }
    if (dilationPixels() > resolution())
    {
        throw std::invalid_argument("Bake dilation cannot exceed the target resolution");
    }
}

reflection::TypeInfo LightmapSettings::settingsType() const
{
    reflection::TypeInfoBuilder<LightmapSettings> type("bg2e.bake.lightmapSettings.v1");
    type.property("resolution", &LightmapSettings::resolution, &LightmapSettings::setResolution).category("Bake").range(1,16384);
    type.property("mode", &LightmapSettings::mode, &LightmapSettings::setMode).category("Bake").enumValue("AO visibility",LightmapMode::RTAO).enumValue("GI multiplier",LightmapMode::RTGI);
    type.property("accumulationFrames", &LightmapSettings::accumulationFrames, &LightmapSettings::setAccumulationFrames).category("Bake").range(1,1048576);
    type.property("samplesPerPixel", &LightmapSettings::samplesPerPixel, &LightmapSettings::setSamplesPerPixel).category("Bake").range(1,64);
    type.property("giBounces", &LightmapSettings::giBounces, &LightmapSettings::setGiBounces).category("Bake").range(1,8);
    type.property("aoRadius", &LightmapSettings::aoRadius, &LightmapSettings::setAoRadius).category("Bake").range(1e-6f,1e9f);
    type.property("maxRayDistance", &LightmapSettings::maxRayDistance, &LightmapSettings::setMaxRayDistance).category("Bake").range(1e-6f,1e9f);
    type.property("rayBias", &LightmapSettings::rayBias, &LightmapSettings::setRayBias).category("Bake").range(1e-6f,1e9f);
    type.property("exposureEV", &LightmapSettings::exposureEV, &LightmapSettings::setExposureEV).category("Bake").range(-32,32);
    type.property("cpuFormat", &LightmapSettings::cpuFormat, &LightmapSettings::setCpuFormat).category("Bake").enumValue("Linear RGB8",LightmapPixelFormat::RGB8).enumValue("Linear RGB32F",LightmapPixelFormat::RGB32F);
    type.property("dilationPixels", &LightmapSettings::dilationPixels, &LightmapSettings::setDilationPixels).category("Bake").range(0,16384);
    return type.build();
}

void* LightmapSettings::settingsInstance()
{
    return this;
}

uint32_t LightmapSettings::resolution() const
{
    return _resolution;
}

void LightmapSettings::setResolution(uint32_t value)
{
    if (value < 1 || value > 16384)
    {
        throw std::invalid_argument("Invalid bake resolution");
    }
    _resolution = value;
}

LightmapMode LightmapSettings::mode() const
{
    return _mode;
}

void LightmapSettings::setMode(LightmapMode value)
{
    if (value != LightmapMode::RTAO && value != LightmapMode::RTGI)
    {
        throw std::invalid_argument("Invalid bake mode");
    }
    _mode = value;
}

uint32_t LightmapSettings::accumulationFrames() const
{
    return _accumulationFrames;
}

void LightmapSettings::setAccumulationFrames(uint32_t value)
{
    if (value < 1 || value > 1048576)
    {
        throw std::invalid_argument("Invalid bake accumulationFrames");
    }
    _accumulationFrames = value;
}

uint32_t LightmapSettings::samplesPerPixel() const
{
    return _samplesPerPixel;
}

void LightmapSettings::setSamplesPerPixel(uint32_t value)
{
    if (value < 1 || value > 64)
    {
        throw std::invalid_argument("Invalid bake samplesPerPixel");
    }
    _samplesPerPixel = value;
}

uint32_t LightmapSettings::giBounces() const
{
    return _giBounces;
}

void LightmapSettings::setGiBounces(uint32_t value)
{
    if (value < 1 || value > 8)
    {
        throw std::invalid_argument("Invalid bake giBounces");
    }
    _giBounces = value;
}

float LightmapSettings::aoRadius() const
{
    return _aoRadius;
}

void LightmapSettings::setAoRadius(float value)
{
    if (!std::isfinite(value) || value < 1e-6f || value > 1e9f)
    {
        throw std::invalid_argument("Invalid bake aoRadius");
    }
    _aoRadius = value;
}

float LightmapSettings::maxRayDistance() const
{
    return _maxRayDistance;
}

void LightmapSettings::setMaxRayDistance(float value)
{
    if (!std::isfinite(value) || value < 1e-6f || value > 1e9f)
    {
        throw std::invalid_argument("Invalid bake maxRayDistance");
    }
    _maxRayDistance = value;
}

float LightmapSettings::rayBias() const
{
    return _rayBias;
}

void LightmapSettings::setRayBias(float value)
{
    if (!std::isfinite(value) || value < 1e-6f || value > 1e9f)
    {
        throw std::invalid_argument("Invalid bake rayBias");
    }
    _rayBias = value;
}

float LightmapSettings::exposureEV() const
{
    return _exposureEV;
}

void LightmapSettings::setExposureEV(float value)
{
    if (!std::isfinite(value) || value < -32 || value > 32)
    {
        throw std::invalid_argument("Invalid bake exposureEV");
    }
    _exposureEV = value;
}

LightmapPixelFormat LightmapSettings::cpuFormat() const
{
    return _cpuFormat;
}

void LightmapSettings::setCpuFormat(LightmapPixelFormat value)
{
    if (value != LightmapPixelFormat::RGB8 && value != LightmapPixelFormat::RGB32F)
    {
        throw std::invalid_argument("Invalid bake cpuFormat");
    }
    _cpuFormat = value;
}

uint32_t LightmapSettings::dilationPixels() const
{
    return _dilationPixels;
}

void LightmapSettings::setDilationPixels(uint32_t value)
{
    if (value > 16384)
    {
        throw std::invalid_argument("Invalid bake dilationPixels");
    }
    _dilationPixels = value;
}
}
