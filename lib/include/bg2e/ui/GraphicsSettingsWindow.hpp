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

#include <bg2e/ui/Window.hpp>
#include <bg2e/ui/EditInteraction.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <array>
#include <variant>
#include <vector>

namespace bg2e::ui {

// UI-only profiles. Enum identities match the reflected settings catalog, not
// the global preset index. Each profile includes hidden values explicitly.
struct GraphicsQualitySetting {
    const char* component;
    const char* property;
    std::variant<bool, uint32_t, float, int64_t> value;
};

struct GraphicsQualityPreset {
    const char* name;
    // Choose the first supported method (MetalFX on Metal, FSR on Vulkan).
    std::array<int64_t, 2> upscaleMethods;
    std::vector<GraphicsQualitySetting> settings;
};

inline const auto graphicsQualityPresets = std::to_array<GraphicsQualityPreset>({
    {"Low", {2, 1}, {
        {"shadows", "enabled", true},
        {"shadows", "bias", 0.002f},
        {"shadows", "directionalDistance", 1000000.0f},
        {"shadows", "maxSamples", 4u},
        {"ambientOcclusion", "enabled", true},
        {"ambientOcclusion", "quality", int64_t{2}},
        {"ambientOcclusion", "sampleCount", 8u},
        {"ambientOcclusion", "bias", 0.002f},
        {"ambientOcclusion", "useBlueNoise", true},
        {"ambientOcclusion", "bounceCount", 2u},
        {"ambientOcclusion", "radius", 0.56f},
        {"ambientOcclusion", "falloff", 1.0f},
        {"ambientOcclusion", "bounceAttenuation", 0.35f},
        {"globalIllumination", "enabled", true},
        {"globalIllumination", "quality", int64_t{0}},
        {"globalIllumination", "sampleCount", 4u},
        {"globalIllumination", "bias", 0.002f},
        {"globalIllumination", "useBlueNoise", true},
        {"globalIllumination", "bounceCount", 2u},
        {"globalIllumination", "maxDistance", 50.0f},
        {"globalIllumination", "shadowSamples", 1u},
        {"globalIllumination", "useShadows", true},
        {"reflections", "enabled", false},
        {"reflections", "quality", int64_t{2}},
        {"reflections", "sampleCount", 6u},
        {"reflections", "bias", 0.002f},
        {"reflections", "useBlueNoise", true},
        {"reflections", "maxDistance", 50.0f},
        {"reflections", "shadowSamples", 1u},
        {"reflections", "useShadows", true},
        {"reflections", "maxRoughness", 0.6f},
        {"reflections", "roughnessSpread", 1.0f},
        {"temporal", "enabled", true},
        {"temporal", "freezeAtLimit", true},
        {"temporal", "maxFrames", 120u},
        {"temporal", "absolutePositionThreshold", 0.02f},
        {"temporal", "relativePositionThreshold", 0.1f},
        {"temporal", "normalThreshold", 0.9f},
        {"temporal", "adaptiveEnabled", false},
        {"temporal", "minCurrentWeight", 0.031f},
        {"temporal", "reactiveCurrentWeight", 0.25f},
        {"temporal", "historyClipSigma", 1.0f},
        {"denoise", "enabled", true},
        {"denoise", "kernelRadius", 2u},
        {"denoise", "depthThreshold", 0.1f},
        {"denoise", "normalThreshold", 0.8f},
        {"denoise", "depthSigma", 0.1f},
        {"denoise", "normalSigma", 0.3f},
        {"denoise", "spatialSigma", 2.0f},
        {"renderer", "indirectLighting", int64_t{1}},
        {"upscaling", "mode", int64_t{3}},
    }},
    {"Medium", {2, 1}, {
        {"shadows", "enabled", true},
        {"shadows", "bias", 0.002f},
        {"shadows", "directionalDistance", 1000000.0f},
        {"shadows", "maxSamples", 8u},
        {"ambientOcclusion", "enabled", true},
        {"ambientOcclusion", "quality", int64_t{2}},
        {"ambientOcclusion", "sampleCount", 8u},
        {"ambientOcclusion", "bias", 0.002f},
        {"ambientOcclusion", "useBlueNoise", true},
        {"ambientOcclusion", "bounceCount", 2u},
        {"ambientOcclusion", "radius", 0.56f},
        {"ambientOcclusion", "falloff", 1.0f},
        {"ambientOcclusion", "bounceAttenuation", 0.35f},
        {"globalIllumination", "enabled", true},
        {"globalIllumination", "quality", int64_t{0}},
        {"globalIllumination", "sampleCount", 4u},
        {"globalIllumination", "bias", 0.002f},
        {"globalIllumination", "useBlueNoise", true},
        {"globalIllumination", "bounceCount", 2u},
        {"globalIllumination", "maxDistance", 100.0f},
        {"globalIllumination", "shadowSamples", 1u},
        {"globalIllumination", "useShadows", true},
        {"reflections", "enabled", true},
        {"reflections", "quality", int64_t{2}},
        {"reflections", "sampleCount", 4u},
        {"reflections", "bias", 0.002f},
        {"reflections", "useBlueNoise", true},
        {"reflections", "maxDistance", 100.0f},
        {"reflections", "shadowSamples", 1u},
        {"reflections", "useShadows", true},
        {"reflections", "maxRoughness", 0.6f},
        {"reflections", "roughnessSpread", 1.0f},
        {"temporal", "enabled", true},
        {"temporal", "freezeAtLimit", true},
        {"temporal", "maxFrames", 120u},
        {"temporal", "absolutePositionThreshold", 0.02f},
        {"temporal", "relativePositionThreshold", 0.1f},
        {"temporal", "normalThreshold", 0.9f},
        {"temporal", "adaptiveEnabled", false},
        {"temporal", "minCurrentWeight", 0.031f},
        {"temporal", "reactiveCurrentWeight", 0.25f},
        {"temporal", "historyClipSigma", 1.0f},
        {"denoise", "enabled", true},
        {"denoise", "kernelRadius", 2u},
        {"denoise", "depthThreshold", 0.1f},
        {"denoise", "normalThreshold", 0.8f},
        {"denoise", "depthSigma", 0.1f},
        {"denoise", "normalSigma", 0.3f},
        {"denoise", "spatialSigma", 2.0f},
        {"renderer", "indirectLighting", int64_t{2}},
        {"upscaling", "mode", int64_t{2}},
    }},
    {"High", {2, 1}, {
        {"shadows", "enabled", true},
        {"shadows", "bias", 0.002f},
        {"shadows", "directionalDistance", 1000000.0f},
        {"shadows", "maxSamples", 16u},
        {"ambientOcclusion", "enabled", true},
        {"ambientOcclusion", "quality", int64_t{2}},
        {"ambientOcclusion", "sampleCount", 8u},
        {"ambientOcclusion", "bias", 0.002f},
        {"ambientOcclusion", "useBlueNoise", true},
        {"ambientOcclusion", "bounceCount", 2u},
        {"ambientOcclusion", "radius", 0.56f},
        {"ambientOcclusion", "falloff", 1.0f},
        {"ambientOcclusion", "bounceAttenuation", 0.35f},
        {"globalIllumination", "enabled", true},
        {"globalIllumination", "quality", int64_t{1}},
        {"globalIllumination", "sampleCount", 8u},
        {"globalIllumination", "bias", 0.002f},
        {"globalIllumination", "useBlueNoise", true},
        {"globalIllumination", "bounceCount", 2u},
        {"globalIllumination", "maxDistance", 1000.0f},
        {"globalIllumination", "shadowSamples", 1u},
        {"globalIllumination", "useShadows", true},
        {"reflections", "enabled", true},
        {"reflections", "quality", int64_t{2}},
        {"reflections", "sampleCount", 6u},
        {"reflections", "bias", 0.002f},
        {"reflections", "useBlueNoise", true},
        {"reflections", "maxDistance", 1000.0f},
        {"reflections", "shadowSamples", 1u},
        {"reflections", "useShadows", true},
        {"reflections", "maxRoughness", 0.7f},
        {"reflections", "roughnessSpread", 1.0f},
        {"temporal", "enabled", true},
        {"temporal", "freezeAtLimit", true},
        {"temporal", "maxFrames", 120u},
        {"temporal", "absolutePositionThreshold", 0.02f},
        {"temporal", "relativePositionThreshold", 0.1f},
        {"temporal", "normalThreshold", 0.9f},
        {"temporal", "adaptiveEnabled", false},
        {"temporal", "minCurrentWeight", 0.031f},
        {"temporal", "reactiveCurrentWeight", 0.25f},
        {"temporal", "historyClipSigma", 1.0f},
        {"denoise", "enabled", true},
        {"denoise", "kernelRadius", 2u},
        {"denoise", "depthThreshold", 0.1f},
        {"denoise", "normalThreshold", 0.8f},
        {"denoise", "depthSigma", 0.1f},
        {"denoise", "normalSigma", 0.3f},
        {"denoise", "spatialSigma", 2.0f},
        {"renderer", "indirectLighting", int64_t{2}},
        {"upscaling", "mode", int64_t{1}},
    }},
    {"Ultra", {2, 1}, {
        {"shadows", "enabled", true},
        {"shadows", "bias", 0.002f},
        {"shadows", "directionalDistance", 1000000.0f},
        {"shadows", "maxSamples", 32u},
        {"ambientOcclusion", "enabled", true},
        {"ambientOcclusion", "quality", int64_t{2}},
        {"ambientOcclusion", "sampleCount", 8u},
        {"ambientOcclusion", "bias", 0.002f},
        {"ambientOcclusion", "useBlueNoise", true},
        {"ambientOcclusion", "bounceCount", 2u},
        {"ambientOcclusion", "radius", 0.56f},
        {"ambientOcclusion", "falloff", 1.0f},
        {"ambientOcclusion", "bounceAttenuation", 0.35f},
        {"globalIllumination", "enabled", true},
        {"globalIllumination", "quality", int64_t{2}},
        {"globalIllumination", "sampleCount", 8u},
        {"globalIllumination", "bias", 0.002f},
        {"globalIllumination", "useBlueNoise", true},
        {"globalIllumination", "bounceCount", 2u},
        {"globalIllumination", "maxDistance", 1000.0f},
        {"globalIllumination", "shadowSamples", 4u},
        {"globalIllumination", "useShadows", true},
        {"reflections", "enabled", true},
        {"reflections", "quality", int64_t{2}},
        {"reflections", "sampleCount", 8u},
        {"reflections", "bias", 0.002f},
        {"reflections", "useBlueNoise", true},
        {"reflections", "maxDistance", 1000.0f},
        {"reflections", "shadowSamples", 4u},
        {"reflections", "useShadows", true},
        {"reflections", "maxRoughness", 0.8f},
        {"reflections", "roughnessSpread", 1.0f},
        {"temporal", "enabled", true},
        {"temporal", "freezeAtLimit", true},
        {"temporal", "maxFrames", 120u},
        {"temporal", "absolutePositionThreshold", 0.02f},
        {"temporal", "relativePositionThreshold", 0.1f},
        {"temporal", "normalThreshold", 0.9f},
        {"temporal", "adaptiveEnabled", false},
        {"temporal", "minCurrentWeight", 0.031f},
        {"temporal", "reactiveCurrentWeight", 0.25f},
        {"temporal", "historyClipSigma", 1.0f},
        {"denoise", "enabled", true},
        {"denoise", "kernelRadius", 2u},
        {"denoise", "depthThreshold", 0.1f},
        {"denoise", "normalThreshold", 0.8f},
        {"denoise", "depthSigma", 0.1f},
        {"denoise", "normalSigma", 0.3f},
        {"denoise", "spatialSigma", 2.0f},
        {"renderer", "indirectLighting", int64_t{2}},
        {"upscaling", "mode", int64_t{0}},
    }},
});

// UI view over a shared live facade. Connect the facade to the delegate before
// its first renderer binding. No renderer pointers or configuration copies live
// here apart from immutable UI quality profiles. UI calls/programmatic methods
// run on the render thread; edits are queued.
class BG2E_API GraphicsSettingsWindow : public Window {
  public:
    GraphicsSettingsWindow();
    ~GraphicsSettingsWindow() override;
    GraphicsSettingsWindow(const GraphicsSettingsWindow&) = delete;
    GraphicsSettingsWindow& operator=(const GraphicsSettingsWindow&) = delete;

    void bind(std::shared_ptr<draw::RenderSettingsPreferences> preferences);
    void clear();
    void setSceneEditInteractionsEnabled(bool enabled);
    bool commitInteraction();
    void cancelInteraction();
    void draw() override;
    void queueEdit(draw::SettingHandle handle, std::any value);
    void importFile(const std::filesystem::path& path,
        draw::UnknownSettingPolicy policy = draw::UnknownSettingPolicy::Reject);
    void exportFile(const std::filesystem::path& path);
    void persist();
    void restore(draw::UnknownSettingPolicy policy = draw::UnknownSettingPolicy::Reject);
    bool pending() const;
    uint64_t catalogGeneration() const;
    const draw::SettingsResult& lastResult() const;

  private:
    struct State;
    std::shared_ptr<State> _state;
    std::unique_ptr<EditSession> _sceneSession;
    std::shared_ptr<draw::RenderSettingsPreferences> _preferences;
    draw::RenderSettingsPreferences::Completion completion();
    void report(const std::string& code, const std::string& message);
    void drawContents();
    void applyPreset(size_t index);
    void queueChanges(std::vector<draw::SettingChange> changes);
};

}
