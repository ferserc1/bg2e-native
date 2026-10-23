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

#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/gpu/metal/MetalFXBridge.hpp>
#include <vector>

namespace bg2e::draw {
class BG2E_API MetalFXUpscaler : public Upscaler {
  public:
    std::string identifier() const override;
    std::string label() const override;
    UpscalerRequirements requirements() const override;
    reflection::TypeInfo settingsType() const override;

    void initialize(Engine&, const UpscalerConfiguration&) override;
    void resize(const UpscalerConfiguration&) override;
    void cleanup() override;
    bool needsRebuild() const override;

    bool enabled() const;
    void setEnabled(bool value);
    bool available() const override;
    bool sdkAvailable() const;
    bool backendSupported() const;
    bool opaqueMotionAvailable() const;
    const std::string& opaqueMotionStatus() const;
    std::string status() const override;

    glm::mat4 prepare(const UpscalerCamera&, uint64_t sceneFrameNumber) override;
    bool record(const UpscalerInput&) override;
    void reset() override;

  private:
    struct Slot {
        std::shared_ptr<gpu::Image> motion, depth;
        std::vector<UpscalerOpaqueMotion> inputs;
        std::shared_ptr<const gpu::detail::RecordingCompletion> recording;
        uint64_t serial = 0;
        bool prepared = false;
    };
    Engine* _engine = nullptr;
    UpscalerConfiguration _configuration;
    gpu::metal::MetalFXBridge _bridge;
    pipeline::FullscreenPass _motion, _opaqueMotion, _depth;
    std::vector<Slot> _slots;
    std::shared_ptr<const gpu::detail::RecordingCompletion> _previousRecording;
    glm::mat4 _previousViewProjection{1}, _previousProjection{1}, _recordedProjection{1};
    bool _enabled = true, _builtEnabled = true, _reset = true, _failed = false;
    bool _backendSupported = false, _cameraSupported = true, _opaqueMotionReady = false;
    uint64_t _lastSceneFrame = 0;
    std::string _error, _opaqueMotionStatus = "Not initialized";
};
}
