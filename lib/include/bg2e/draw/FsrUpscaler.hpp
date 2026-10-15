#pragma once

#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/gpu/vk/FidelityFXBridge.hpp>

namespace bg2e::draw {
class BG2E_API FsrUpscaler : public Upscaler {
  public:
    std::string identifier() const override;
    std::string label() const override;
    UpscalerRequirements requirements() const override;
    void initialize(Engine&, const UpscalerConfiguration&) override;
    void resize(const UpscalerConfiguration&) override;
    bool available() const override;
    std::string status() const override;
    bool sdkAvailable() const;
    bool backendSupported() const;
    std::string hardwareStatus() const;
    bool enabled() const;
    void setEnabled(bool);
    float sharpness() const;
    void setSharpness(float);
    glm::mat4 prepare(const UpscalerCamera&, uint64_t sceneFrameNumber) override;
    bool record(const UpscalerInput&) override;
    void reset() override;
    void cleanup() override;
    reflection::TypeInfo settingsType() const override;
    bool needsRebuild() const override;
  private:
    Engine* _engine = nullptr;
    UpscalerConfiguration _configuration;
    gpu::vk::FidelityFXBridge _bridge;
    pipeline::FullscreenPass _motion;
    std::vector<std::shared_ptr<gpu::Image>> _motionImages;
    glm::mat4 _previousViewProjection{1}, _previousProjection{1};
    float _jitterX = 0, _jitterY = 0;
    float _sharpness = 0.5f;
    bool _enabled = true, _builtEnabled = true, _reset = true, _failed = false, _cameraSupported = true;
    uint64_t _lastSceneFrame = 0;
    std::string _error;
};
}
