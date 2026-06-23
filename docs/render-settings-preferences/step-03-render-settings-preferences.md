# Step 03: bg2e::render::RenderSettingsPreferences

## Files to Create

- `lib/include/bg2e/render/RenderSettingsPreferences.hpp`
- `lib/src/bg2e/render/RenderSettingsPreferences.cpp`

## Files to Modify

- `lib/include/bg2e/render/all.hpp` — add include

## Interface

```cpp
// lib/include/bg2e/render/RenderSettingsPreferences.hpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/app/Preferences.hpp>
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
```

## Implementation Details

### Constructor

```cpp
RenderSettingsPreferences::RenderSettingsPreferences(RendererDeferred* renderer)
    : _renderer(renderer)
    , _prefs("render")  // scoped to preferences_render.json
{
}
```

### `load()`

Reads saved values from disk and applies them to the renderer. Uses current renderer values as defaults for missing keys.

```cpp
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
```

### `persist()`

Reads all current values from the renderer and writes to disk if dirty.

```cpp
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
```

### Getters (read directly from renderer)

```cpp
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
```

### Setters (write to renderer + mark dirty)

Each setter follows this pattern — example for a few representative ones:

```cpp
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

void RenderSettingsPreferences::setAORadius(float v)
{
    _renderer->setAORadius(v);
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

void RenderSettingsPreferences::setTemporalMode(uint32_t v)
{
    _renderer->setTemporalMode(
        static_cast<deferred::TemporalAccumulator::AccumulationMode>(v));
    _dirty = true;
}

// ... remaining setters follow the same pattern
```

Full setter list (all 31 setters):

| Setter | Type | Renderer call |
|--------|------|---------------|
| `setRenderScaleIndex(uint32_t)` | `uint32_t` | `setScaleOption(v)` |
| `setIndirectLightingMode(uint32_t)` | `uint32_t` | `setIndirectLightingMode(static_cast<IndirectLightingMode>(v))` |
| `setAOQualityIndex(uint32_t)` | `uint32_t` | `setAOQuality(static_cast<RTAOQuality>(v))` |
| `setAOSampleCount(int)` | `int` | `setAOSampleCount(v)` |
| `setAOBounceCount(int)` | `int` | `setAOBounceCount(v)` |
| `setAORadius(float)` | `float` | `setAORadius(v)` |
| `setAOBias(float)` | `float` | `setAOBias(v)` |
| `setAOFalloff(float)` | `float` | `setAOFalloff(v)` |
| `setAOBounceAttenuation(float)` | `float` | `setAOBounceAttenuation(v)` |
| `setRTGIEnabled(bool)` | `bool` | `setRTGIEnabled(v)` |
| `setRTGIQualityIndex(uint32_t)` | `uint32_t` | `setRTGIQuality(static_cast<RTGIQuality>(v))` |
| `setRTGISampleCount(int)` | `int` | `setRTGISampleCount(static_cast<uint32_t>(v))` |
| `setRTGIBounceCount(int)` | `int` | `setRTGIBounceCount(static_cast<uint32_t>(v))` |
| `setRTGIRayBias(float)` | `float` | `setRTGIRayBias(v)` |
| `setRTGIMaxDistance(float)` | `float` | `setRTGIMaxDistance(v)` |
| `setRTReflectionsEnabled(bool)` | `bool` | `setRTReflectionsEnabled(v)` |
| `setRTReflectionSampleCount(int)` | `int` | `setRTReflectionSampleCount(v)` |
| `setRTReflectionMaxRoughness(float)` | `float` | `setRTReflectionMaxRoughness(v)` |
| `setRTReflectionRayBias(float)` | `float` | `setRTReflectionRayBias(v)` |
| `setRTReflectionMaxDistance(float)` | `float` | `setRTReflectionMaxDistance(v)` |
| `setRTReflectionRoughnessSpread(float)` | `float` | `setRTReflectionRoughnessSpread(v)` |
| `setTemporalMode(uint32_t)` | `uint32_t` | `setTemporalMode(static_cast<AccumulationMode>(v))` |
| `setTemporalHistoryWeight(float)` | `float` | `setTemporalHistoryWeight(v)` |
| `setTemporalDepthThreshold(float)` | `float` | `setTemporalDepthThreshold(v)` |
| `setTemporalNormalThreshold(float)` | `float` | `setTemporalNormalThreshold(v)` |
| `setDenoiseKernelRadius(int)` | `int` | `setDenoiseKernelRadius(v)` |
| `setDenoiseDepthThreshold(float)` | `float` | `setDenoiseDepthThreshold(v)` |
| `setDenoiseNormalThreshold(float)` | `float` | `setDenoiseNormalThreshold(v)` |
| `setDenoiseDepthSigma(float)` | `float` | `setDenoiseDepthSigma(v)` |
| `setDenoiseNormalSigma(float)` | `float` | `setDenoiseNormalSigma(v)` |

## Integration Points

- Used by `UIRenderSettingsWindow` (Step 04) for reading and writing settings
- Used by both app `AppDelegate` classes (Steps 05/06) for initialization, timer registration, and exit persistence
