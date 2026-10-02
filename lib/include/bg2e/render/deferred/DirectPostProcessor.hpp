/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 */

#pragma once

#include <bg2e/render/deferred/FinalPostProcessor.hpp>

namespace bg2e::render::deferred {

// Native-resolution output path with no anti-aliasing, temporal jitter,
// sharpening, or scaling. The only operation is the copy required to move the
// deferred result into the final output image.
class BG2E_API DirectPostProcessor : public FinalPostProcessor {
public:
    bool build(Engine* engine, VkExtent2D renderExtent, VkExtent2D displayExtent,
               VkFormat colorFormat) override;
    void resize(VkExtent2D renderExtent, VkExtent2D displayExtent) override;
    glm::mat4 prepare(const glm::mat4& projMatrix, uint32_t frameCounter,
                      VkExtent2D renderExtent) override;
    void process(VkCommandBuffer cmd, uint32_t frameIndex,
                 const vulkan::Image* colorInput, const vulkan::Image* depthInput,
                 const vulkan::Image* motionVectors, const vulkan::Image* colorOutput,
                 float deltaMs, float cameraNear, float cameraFar,
                 float cameraFovVertical) override;
    void cleanup() override {}

    std::string processorName() const override { return "Final Rendering"; }
    std::vector<std::string> scaleOptions() const override { return { "Direct (100%, No AA)" }; }
    void setScaleOption(uint32_t) override {}
    uint32_t scaleOption() const override { return 0; }
    float renderScalePercent() const override { return 100.0f; }

private:
    VkExtent2D _renderExtent{};
    VkExtent2D _displayExtent{};
};

}
