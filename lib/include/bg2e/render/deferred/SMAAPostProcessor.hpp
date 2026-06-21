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

#include <bg2e/render/deferred/FinalPostProcessor.hpp>
#include <bg2e/render/deferred/SMAAProcessor.hpp>

namespace bg2e {
namespace render {
namespace deferred {

// FinalPostProcessor implementation using SMAA anti-aliasing.
// Available on all platforms. Does not use depth or motion vectors.
// Jitter is not applied (prepare() returns projMatrix unchanged).
class BG2E_API SMAAPostProcessor : public FinalPostProcessor {
public:
    SMAAPostProcessor() = default;
    ~SMAAPostProcessor() override = default;

    void build(
        Engine* engine,
        VkExtent2D renderExtent,
        VkExtent2D displayExtent,
        VkFormat colorFormat
    ) override;

    void resize(VkExtent2D renderExtent, VkExtent2D displayExtent) override;

    glm::mat4 prepare(
        const glm::mat4& projMatrix,
        uint32_t frameCounter,
        VkExtent2D renderExtent
    ) override;

    void process(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        const vulkan::Image* colorInput,
        const vulkan::Image* depthInput,
        const vulkan::Image* motionVectors,
        const vulkan::Image* colorOutput,
        float deltaMs,
        float cameraNear,
        float cameraFar,
        float cameraFovVertical
    ) override;

    void cleanup() override;

    std::string processorName() const override { return "Render Scale"; }
    std::vector<std::string> scaleOptions() const override;
    void setScaleOption(uint32_t index) override;
    uint32_t scaleOption() const override { return _scaleOptionIndex; }
    float renderScalePercent() const override;

private:
    static constexpr float kScaleValues[] = { 25.0f, 50.0f, 75.0f, 100.0f, 150.0f };

    Engine*                         _engine            = nullptr;
    VkExtent2D                      _renderExtent      = {};
    VkExtent2D                      _displayExtent     = {};
    uint32_t                        _scaleOptionIndex  = 1; // default: 50%
    std::unique_ptr<SMAAProcessor>  _smaa;
};

}
}
}
