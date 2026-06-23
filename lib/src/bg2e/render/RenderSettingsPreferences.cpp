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

#include <bg2e/render/RenderSettingsPreferences.hpp>
#include <bg2e/render/RendererDeferred.hpp>

namespace bg2e {
namespace render {

RenderSettingsPreferences::RenderSettingsPreferences(RendererDeferred* renderer)
    : _renderer(renderer)
    , _prefs("render")
{
}

void RenderSettingsPreferences::load()
{
    // Render Scale
    _renderer->setScaleOption(
        _prefs.get("render_scale_optionIndex", _renderer->scaleOption()));

    // Indirect Lighting Mode
    _renderer->setIndirectLightingMode(
        static_cast<deferred::IndirectLightingMode>(
            _prefs.get("render_il_mode",
                static_cast<uint32_t>(_renderer->indirectLightingMode()))));

    // RTAO
    _renderer->setAOQuality(static_cast<deferred::RTAOQuality>(
        _prefs.get("render_ao_qualityIndex",
            static_cast<uint32_t>(_renderer->aoQuality()))));
    _renderer->setAOSampleCount(
        _prefs.get("render_ao_sampleCount", _renderer->aoSampleCount()));
    _renderer->setAOBounceCount(
        _prefs.get("render_ao_bounceCount", _renderer->aoBounceCount()));
    _renderer->setAORadius(
        _prefs.get("render_ao_radius", _renderer->aoRadius()));
    _renderer->setAOBias(
        _prefs.get("render_ao_bias", _renderer->aoBias()));
    _renderer->setAOFalloff(
        _prefs.get("render_ao_falloff", _renderer->aoFalloff()));
    _renderer->setAOBounceAttenuation(
        _prefs.get("render_ao_bounceAttenuation", _renderer->aoBounceAttenuation()));

    // RTGI
    _renderer->setRTGIEnabled(
        _prefs.get("render_gi_enabled", _renderer->rtGIEnabled()));
    _renderer->setRTGIQuality(static_cast<deferred::RTGIQuality>(
        _prefs.get("render_gi_qualityIndex",
            static_cast<uint32_t>(_renderer->rtGIQuality()))));
    _renderer->setRTGISampleCount(
        _prefs.get("render_gi_sampleCount", _renderer->rtGISampleCount()));
    _renderer->setRTGIBounceCount(
        _prefs.get("render_gi_bounceCount", _renderer->rtGIBounceCount()));
    _renderer->setRTGIRayBias(
        _prefs.get("render_gi_rayBias", _renderer->rtGIRayBias()));
    _renderer->setRTGIMaxDistance(
        _prefs.get("render_gi_maxDistance", _renderer->rtGIMaxDistance()));

    // RT Reflections
    _renderer->setRTReflectionsEnabled(
        _prefs.get("render_reflect_enabled", _renderer->rtReflectionsEnabled()));
    _renderer->setRTReflectionSampleCount(
        _prefs.get("render_reflect_sampleCount", _renderer->rtReflectionSampleCount()));
    _renderer->setRTReflectionMaxRoughness(
        _prefs.get("render_reflect_maxRoughness", _renderer->rtReflectionMaxRoughness()));
    _renderer->setRTReflectionRayBias(
        _prefs.get("render_reflect_rayBias", _renderer->rtReflectionRayBias()));
    _renderer->setRTReflectionMaxDistance(
        _prefs.get("render_reflect_maxDistance", _renderer->rtReflectionMaxDistance()));
    _renderer->setRTReflectionRoughnessSpread(
        _prefs.get("render_reflect_roughnessSpread", _renderer->rtReflectionRoughnessSpread()));

    // Temporal
    _renderer->setTemporalMode(static_cast<deferred::TemporalAccumulator::AccumulationMode>(
        _prefs.get("render_ta_mode",
            static_cast<uint32_t>(_renderer->temporalMode()))));
    _renderer->setTemporalHistoryWeight(
        _prefs.get("render_ta_historyWeight", _renderer->temporalHistoryWeight()));
    _renderer->setTemporalDepthThreshold(
        _prefs.get("render_ta_depthThreshold", _renderer->temporalDepthThreshold()));
    _renderer->setTemporalNormalThreshold(
        _prefs.get("render_ta_normalThreshold", _renderer->temporalNormalThreshold()));

    // Denoise
    _renderer->setDenoiseKernelRadius(
        _prefs.get("render_denoise_kernRadius", _renderer->denoiseKernelRadius()));
    _renderer->setDenoiseDepthThreshold(
        _prefs.get("render_denoise_depthThreshold", _renderer->denoiseDepthThreshold()));
    _renderer->setDenoiseNormalThreshold(
        _prefs.get("render_denoise_normalThreshold", _renderer->denoiseNormalThreshold()));
    _renderer->setDenoiseDepthSigma(
        _prefs.get("render_denoise_depthSigma", _renderer->denoiseDepthSigma()));
    _renderer->setDenoiseNormalSigma(
        _prefs.get("render_denoise_normalSigma", _renderer->denoiseNormalSigma()));

    _dirty = false;
}

void RenderSettingsPreferences::persist()
{
    if (!_dirty) return;

    _prefs.set("render_scale_optionIndex", _renderer->scaleOption());
    _prefs.set("render_il_mode", static_cast<uint32_t>(_renderer->indirectLightingMode()));

    _prefs.set("render_ao_qualityIndex", static_cast<uint32_t>(_renderer->aoQuality()));
    _prefs.set("render_ao_sampleCount", _renderer->aoSampleCount());
    _prefs.set("render_ao_bounceCount", _renderer->aoBounceCount());
    _prefs.set("render_ao_radius", _renderer->aoRadius());
    _prefs.set("render_ao_bias", _renderer->aoBias());
    _prefs.set("render_ao_falloff", _renderer->aoFalloff());
    _prefs.set("render_ao_bounceAttenuation", _renderer->aoBounceAttenuation());

    _prefs.set("render_gi_enabled", _renderer->rtGIEnabled());
    _prefs.set("render_gi_qualityIndex", static_cast<uint32_t>(_renderer->rtGIQuality()));
    _prefs.set("render_gi_sampleCount", _renderer->rtGISampleCount());
    _prefs.set("render_gi_bounceCount", _renderer->rtGIBounceCount());
    _prefs.set("render_gi_rayBias", _renderer->rtGIRayBias());
    _prefs.set("render_gi_maxDistance", _renderer->rtGIMaxDistance());

    _prefs.set("render_reflect_enabled", _renderer->rtReflectionsEnabled());
    _prefs.set("render_reflect_sampleCount", _renderer->rtReflectionSampleCount());
    _prefs.set("render_reflect_maxRoughness", _renderer->rtReflectionMaxRoughness());
    _prefs.set("render_reflect_rayBias", _renderer->rtReflectionRayBias());
    _prefs.set("render_reflect_maxDistance", _renderer->rtReflectionMaxDistance());
    _prefs.set("render_reflect_roughnessSpread", _renderer->rtReflectionRoughnessSpread());

    _prefs.set("render_ta_mode", static_cast<uint32_t>(_renderer->temporalMode()));
    _prefs.set("render_ta_historyWeight", _renderer->temporalHistoryWeight());
    _prefs.set("render_ta_depthThreshold", _renderer->temporalDepthThreshold());
    _prefs.set("render_ta_normalThreshold", _renderer->temporalNormalThreshold());

    _prefs.set("render_denoise_kernRadius", _renderer->denoiseKernelRadius());
    _prefs.set("render_denoise_depthThreshold", _renderer->denoiseDepthThreshold());
    _prefs.set("render_denoise_normalThreshold", _renderer->denoiseNormalThreshold());
    _prefs.set("render_denoise_depthSigma", _renderer->denoiseDepthSigma());
    _prefs.set("render_denoise_normalSigma", _renderer->denoiseNormalSigma());

    _prefs.save();
    _dirty = false;
}

// --- Getters (read directly from renderer) ---
uint32_t RenderSettingsPreferences::renderScaleIndex() const { return _renderer->scaleOption(); }
uint32_t RenderSettingsPreferences::indirectLightingMode() const { return static_cast<uint32_t>(_renderer->indirectLightingMode()); }
uint32_t RenderSettingsPreferences::aoQualityIndex() const { return static_cast<uint32_t>(_renderer->aoQuality()); }
int RenderSettingsPreferences::aoSampleCount() const { return _renderer->aoSampleCount(); }
int RenderSettingsPreferences::aoBounceCount() const { return _renderer->aoBounceCount(); }
float RenderSettingsPreferences::aoRadius() const { return _renderer->aoRadius(); }
float RenderSettingsPreferences::aoBias() const { return _renderer->aoBias(); }
float RenderSettingsPreferences::aoFalloff() const { return _renderer->aoFalloff(); }
float RenderSettingsPreferences::aoBounceAttenuation() const { return _renderer->aoBounceAttenuation(); }
bool RenderSettingsPreferences::rtGIEnabled() const { return _renderer->rtGIEnabled(); }
uint32_t RenderSettingsPreferences::rtGIQualityIndex() const { return static_cast<uint32_t>(_renderer->rtGIQuality()); }
int RenderSettingsPreferences::rtGISampleCount() const { return static_cast<int>(_renderer->rtGISampleCount()); }
int RenderSettingsPreferences::rtGIBounceCount() const { return static_cast<int>(_renderer->rtGIBounceCount()); }
float RenderSettingsPreferences::rtGIRayBias() const { return _renderer->rtGIRayBias(); }
float RenderSettingsPreferences::rtGIMaxDistance() const { return _renderer->rtGIMaxDistance(); }
bool RenderSettingsPreferences::rtReflectionsEnabled() const { return _renderer->rtReflectionsEnabled(); }
int RenderSettingsPreferences::rtReflectionSampleCount() const { return _renderer->rtReflectionSampleCount(); }
float RenderSettingsPreferences::rtReflectionMaxRoughness() const { return _renderer->rtReflectionMaxRoughness(); }
float RenderSettingsPreferences::rtReflectionRayBias() const { return _renderer->rtReflectionRayBias(); }
float RenderSettingsPreferences::rtReflectionMaxDistance() const { return _renderer->rtReflectionMaxDistance(); }
float RenderSettingsPreferences::rtReflectionRoughnessSpread() const { return _renderer->rtReflectionRoughnessSpread(); }
uint32_t RenderSettingsPreferences::temporalMode() const { return static_cast<uint32_t>(_renderer->temporalMode()); }
float RenderSettingsPreferences::temporalHistoryWeight() const { return _renderer->temporalHistoryWeight(); }
float RenderSettingsPreferences::temporalDepthThreshold() const { return _renderer->temporalDepthThreshold(); }
float RenderSettingsPreferences::temporalNormalThreshold() const { return _renderer->temporalNormalThreshold(); }
int RenderSettingsPreferences::denoiseKernelRadius() const { return _renderer->denoiseKernelRadius(); }
float RenderSettingsPreferences::denoiseDepthThreshold() const { return _renderer->denoiseDepthThreshold(); }
float RenderSettingsPreferences::denoiseNormalThreshold() const { return _renderer->denoiseNormalThreshold(); }
float RenderSettingsPreferences::denoiseDepthSigma() const { return _renderer->denoiseDepthSigma(); }
float RenderSettingsPreferences::denoiseNormalSigma() const { return _renderer->denoiseNormalSigma(); }

// --- Setters (write to renderer + mark dirty) ---
void RenderSettingsPreferences::setRenderScaleIndex(uint32_t v)
{
    _renderer->setScaleOption(v);
    _dirty = true;
}

void RenderSettingsPreferences::setIndirectLightingMode(uint32_t v)
{
    _renderer->setIndirectLightingMode(static_cast<deferred::IndirectLightingMode>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setAOQualityIndex(uint32_t v)
{
    _renderer->setAOQuality(static_cast<deferred::RTAOQuality>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setAOSampleCount(int v)
{
    _renderer->setAOSampleCount(v);
    _dirty = true;
}

void RenderSettingsPreferences::setAOBounceCount(int v)
{
    _renderer->setAOBounceCount(v);
    _dirty = true;
}

void RenderSettingsPreferences::setAORadius(float v)
{
    _renderer->setAORadius(v);
    _dirty = true;
}

void RenderSettingsPreferences::setAOBias(float v)
{
    _renderer->setAOBias(v);
    _dirty = true;
}

void RenderSettingsPreferences::setAOFalloff(float v)
{
    _renderer->setAOFalloff(v);
    _dirty = true;
}

void RenderSettingsPreferences::setAOBounceAttenuation(float v)
{
    _renderer->setAOBounceAttenuation(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTGIEnabled(bool v)
{
    _renderer->setRTGIEnabled(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTGIQualityIndex(uint32_t v)
{
    _renderer->setRTGIQuality(static_cast<deferred::RTGIQuality>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setRTGISampleCount(int v)
{
    _renderer->setRTGISampleCount(static_cast<uint32_t>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setRTGIBounceCount(int v)
{
    _renderer->setRTGIBounceCount(static_cast<uint32_t>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setRTGIRayBias(float v)
{
    _renderer->setRTGIRayBias(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTGIMaxDistance(float v)
{
    _renderer->setRTGIMaxDistance(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionsEnabled(bool v)
{
    _renderer->setRTReflectionsEnabled(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionSampleCount(int v)
{
    _renderer->setRTReflectionSampleCount(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionMaxRoughness(float v)
{
    _renderer->setRTReflectionMaxRoughness(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionRayBias(float v)
{
    _renderer->setRTReflectionRayBias(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionMaxDistance(float v)
{
    _renderer->setRTReflectionMaxDistance(v);
    _dirty = true;
}

void RenderSettingsPreferences::setRTReflectionRoughnessSpread(float v)
{
    _renderer->setRTReflectionRoughnessSpread(v);
    _dirty = true;
}

void RenderSettingsPreferences::setTemporalMode(uint32_t v)
{
    _renderer->setTemporalMode(
        static_cast<deferred::TemporalAccumulator::AccumulationMode>(v));
    _dirty = true;
}

void RenderSettingsPreferences::setTemporalHistoryWeight(float v)
{
    _renderer->setTemporalHistoryWeight(v);
    _dirty = true;
}

void RenderSettingsPreferences::setTemporalDepthThreshold(float v)
{
    _renderer->setTemporalDepthThreshold(v);
    _dirty = true;
}

void RenderSettingsPreferences::setTemporalNormalThreshold(float v)
{
    _renderer->setTemporalNormalThreshold(v);
    _dirty = true;
}

void RenderSettingsPreferences::setDenoiseKernelRadius(int v)
{
    _renderer->setDenoiseKernelRadius(v);
    _dirty = true;
}

void RenderSettingsPreferences::setDenoiseDepthThreshold(float v)
{
    _renderer->setDenoiseDepthThreshold(v);
    _dirty = true;
}

void RenderSettingsPreferences::setDenoiseNormalThreshold(float v)
{
    _renderer->setDenoiseNormalThreshold(v);
    _dirty = true;
}

void RenderSettingsPreferences::setDenoiseDepthSigma(float v)
{
    _renderer->setDenoiseDepthSigma(v);
    _dirty = true;
}

void RenderSettingsPreferences::setDenoiseNormalSigma(float v)
{
    _renderer->setDenoiseNormalSigma(v);
    _dirty = true;
}

}
}