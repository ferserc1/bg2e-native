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

#pragma once

#include <bg2e/bake/Types.hpp>
#include <bg2e/reflection/TypeInfo.hpp>

namespace bg2e::bake {
// Actual per-target settings object. UI binds this instance, never a cached value model.
// Mutate at a stopped job boundary, invalidate the target generation and restart.
// Executor freezes a validated copy at job start; scalar setters allow paired edits.
class BG2E_API LightmapSettings {
  public:
    void validate() const;

    reflection::TypeInfo settingsType() const;
    void* settingsInstance();

    uint32_t resolution() const;
    void setResolution(uint32_t value);
    LightmapMode mode() const;
    void setMode(LightmapMode value);
    uint32_t accumulationFrames() const;
    void setAccumulationFrames(uint32_t value);
    uint32_t samplesPerPixel() const;
    void setSamplesPerPixel(uint32_t value);
    uint32_t giBounces() const;
    void setGiBounces(uint32_t value);
    float aoRadius() const;
    void setAoRadius(float value);
    float maxRayDistance() const;
    void setMaxRayDistance(float value);
    float rayBias() const;
    void setRayBias(float value);
    float exposureEV() const;
    void setExposureEV(float value);
    LightmapPixelFormat cpuFormat() const;
    void setCpuFormat(LightmapPixelFormat value);
    uint32_t dilationPixels() const;
    void setDilationPixels(uint32_t value);

  private:
    uint32_t _resolution = 512;
    LightmapMode _mode = LightmapMode::RTGI;
    uint32_t _accumulationFrames = 16;
    uint32_t _samplesPerPixel = 8;
    uint32_t _giBounces = 2;
    float _aoRadius = 0.1f;
    float _maxRayDistance = 50.0f;
    float _rayBias = 0.0005f;
    float _exposureEV = 0.0f;
    LightmapPixelFormat _cpuFormat = LightmapPixelFormat::RGB8;
    uint32_t _dilationPixels = 4;
};
}
