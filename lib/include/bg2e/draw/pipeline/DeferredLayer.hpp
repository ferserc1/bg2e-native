#pragma once

#include <bg2e/draw/pipeline/FullscreenPass.hpp>
#include <bg2e/draw/pipeline/RasterFrame.hpp>

namespace bg2e::draw::pipeline {
BG2E_API PassContract deferredContract(gpu::PixelFormat depth, bool shadows = false);
BG2E_API gpu::PipelineLayoutDescription deferredBindingLayout(bool shadows = false);
enum class LightingDebugView : uint32_t
{
    Final, Albedo, Direct, Indirect, Emission, NonFinite, Irradiance, Prefiltered, BRDF, Occlusion, Baked
};
class BG2E_API DeferredLayer : public RenderLayer {
  public:
    explicit DeferredLayer(std::filesystem::path shaderRoot = {}, uint32_t slots = 0,
        gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT, bool shadows = false);
    LightingDebugView debugView() const;
    void setDebugView(LightingDebugView value);
    const PassContract& contract() const override;
    void initialize(Engine& engine, const LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const LayerConfiguration&) override;
    void update(const FrameContext& frame) override;
    void record(const PassContext& context) override;
    void cleanup() override;
  protected:
    std::filesystem::path _shaderRoot;
    uint32_t _slots;
    PassContract _contract;
    FullscreenPass _pass;
    LightingDebugView _debugView = LightingDebugView::Final;
};
enum class FinalDebugView : uint32_t
{
    Final, Albedo, NormalEmission, Material, FresnelFlags, SheenRefraction, BakedMultiplier, Depth, RawHDR
};
// Exposure/contrast/brightness and gamma are applied once after HDR composition.
// debug: 0 final, 1 albedo, 2 normal, 3 material, 4 fresnel, 5 sheen, 6 baked, 7 depth, 8 raw reconstructed HDR.
struct alignas(16) FinalSettings {
    glm::vec4 color{0, 1, 0, 2.2f}; // exposure stops, contrast, brightness, gamma
    glm::vec4 outputExtent{0}; // Resolved by FinalLayer at record time.
    glm::uvec4 display{0}; // debug view, hardware sRGB output, reserved, reserved
};
static_assert(sizeof(FinalSettings) == 48);
BG2E_API PassContract finalContract(gpu::PixelFormat output, gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT,
    bool reconstructedColor = false);
class BG2E_API FinalLayer : public RenderLayer {
  public:
    explicit FinalLayer(std::filesystem::path shaderRoot = {}, uint32_t slots = 0,
        gpu::PixelFormat output = gpu::PixelFormat::R8G8B8A8_UNORM, gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT,
        bool reconstructedColor = false);
    FinalSettings settings;
    float exposure() const;
    void setExposure(float value);
    float contrast() const;
    void setContrast(float value);
    float brightness() const;
    void setBrightness(float value);
    float gamma() const;
    void setGamma(float value);
    FinalDebugView debugView() const;
    void setDebugView(FinalDebugView value);
    const PassContract& contract() const override;
    void initialize(Engine& engine, const LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const LayerConfiguration&) override;
    void update(const FrameContext& frame) override;
    void record(const PassContext& context) override;
    void cleanup() override;
  protected:
    std::filesystem::path _shaderRoot;
    uint32_t _slots;
    PassContract _contract;
    FullscreenPass _pass;
};
}
