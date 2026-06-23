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

#include <bg2e/common.hpp>
#include <bg2e/app/Preferences.hpp>
#include <bg2e/render/deferred/DeferredLayer.hpp>
#include <cstdint>

namespace bg2e {
namespace render {

class RendererDeferred;

class BG2E_API RenderSettingsPreferences {
public:
    explicit RenderSettingsPreferences(RendererDeferred* renderer);

    void load();
    void persist();

    // --- Render Scale ---
    uint32_t renderScaleIndex() const;
    void setRenderScaleIndex(uint32_t value);

    // --- Indirect Lighting Mode ---
    uint32_t indirectLightingMode() const;
    void setIndirectLightingMode(uint32_t value);

    // --- RTAO ---
    uint32_t aoQualityIndex() const;
    void setAOQualityIndex(uint32_t value);
    int aoSampleCount() const;
    void setAOSampleCount(int value);
    int aoBounceCount() const;
    void setAOBounceCount(int value);
    float aoRadius() const;
    void setAORadius(float value);
    float aoBias() const;
    void setAOBias(float value);
    float aoFalloff() const;
    void setAOFalloff(float value);
    float aoBounceAttenuation() const;
    void setAOBounceAttenuation(float value);

    // --- RTGI ---
    bool rtGIEnabled() const;
    void setRTGIEnabled(bool value);
    uint32_t rtGIQualityIndex() const;
    void setRTGIQualityIndex(uint32_t value);
    int rtGISampleCount() const;
    void setRTGISampleCount(int value);
    int rtGIBounceCount() const;
    void setRTGIBounceCount(int value);
    float rtGIRayBias() const;
    void setRTGIRayBias(float value);
    float rtGIMaxDistance() const;
    void setRTGIMaxDistance(float value);

    // --- RT Reflections ---
    bool rtReflectionsEnabled() const;
    void setRTReflectionsEnabled(bool value);
    int rtReflectionSampleCount() const;
    void setRTReflectionSampleCount(int value);
    float rtReflectionMaxRoughness() const;
    void setRTReflectionMaxRoughness(float value);
    float rtReflectionRayBias() const;
    void setRTReflectionRayBias(float value);
    float rtReflectionMaxDistance() const;
    void setRTReflectionMaxDistance(float value);
    float rtReflectionRoughnessSpread() const;
    void setRTReflectionRoughnessSpread(float value);

    // --- Temporal Accumulator ---
    uint32_t temporalMode() const;
    void setTemporalMode(uint32_t value);
    float temporalHistoryWeight() const;
    void setTemporalHistoryWeight(float value);
    float temporalDepthThreshold() const;
    void setTemporalDepthThreshold(float value);
    float temporalNormalThreshold() const;
    void setTemporalNormalThreshold(float value);

    // --- Denoise ---
    int denoiseKernelRadius() const;
    void setDenoiseKernelRadius(int value);
    float denoiseDepthThreshold() const;
    void setDenoiseDepthThreshold(float value);
    float denoiseNormalThreshold() const;
    void setDenoiseNormalThreshold(float value);
    float denoiseDepthSigma() const;
    void setDenoiseDepthSigma(float value);
    float denoiseNormalSigma() const;
    void setDenoiseNormalSigma(float value);

private:
    RendererDeferred* _renderer;
    app::Preferences _prefs;
    bool _dirty = false;
};

}
}