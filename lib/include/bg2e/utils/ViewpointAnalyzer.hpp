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
#include <bg2e/draw/Engine.hpp>
#include <bg2e/math/base.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace bg2e::scene {
class BoundingBox;
class DrawableComponent;
class Node;
class Scene;
}

namespace bg2e::gpu {
class Image;
class GraphicsPipeline;
class PipelineLayout;
}

namespace bg2e::utils {

/** A uniformly sampled scalar interval. Cyclic intervals exclude max. */
struct ViewpointSampleRange {
    float min = 0.0f;
    float max = 0.0f;
    uint32_t samples = 1;
    bool cyclic = false;
};

struct ViewpointSamplingConfig {
    uint32_t width = 64;
    uint32_t height = 64;
    // Angular values are in degrees; positive pitch places the camera above target.
    ViewpointSampleRange yaw{0.0f, 360.0f, 18, true};
    ViewpointSampleRange pitch{5.0f, 35.0f, 4, false};
    ViewpointSampleRange targetHeight{0.5f, 0.5f, 1, false};
    ViewpointSampleRange distanceScale{1.1f, 1.1f, 1, false};
    ViewpointSampleRange fieldOfView{35.0f, 35.0f, 1, false};
};

struct CameraCandidate {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float targetHeight = 0.5f;
    float distanceScale = 1.1f;
    float fieldOfView = 35.0f;
    float distance = 0.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    glm::vec3 position{0.0f};
    glm::vec3 target{0.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
};

struct ViewAnalysis {
    uint32_t visiblePixelCount = 0;
    uint32_t borderPixelCount = 0;
    uint32_t visibleSubmeshCount = 0;
    glm::uvec2 minPixel{0, 0};
    glm::uvec2 maxPixel{0, 0};
    float minDepth = 0.0f;
    float maxDepth = 0.0f;
    float meanDepth = 0.0f;
    float depthVariance = 0.0f;
    std::vector<uint32_t> pixelsPerSubmesh;
};

struct ViewSample {
    CameraCandidate camera;
    ViewAnalysis analysis;
    float score = 0.0f;
};

struct AnalyzedSubmesh {
    uint32_t identifier = 0;
    std::weak_ptr<scene::Node> node;
    std::weak_ptr<scene::DrawableComponent> drawable;
    uint32_t submesh = 0;
};

/** Synchronous draw/GPU-backed candidate view analyzer. The engine and scene
 * resources are borrowed; stop scene and GPU producers while analyze runs. */
class BG2E_API ViewpointAnalyzer {
  public:
    explicit ViewpointAnalyzer(draw::Engine* engine);
    virtual ~ViewpointAnalyzer();

    ViewpointAnalyzer(const ViewpointAnalyzer&) = delete;
    ViewpointAnalyzer& operator=(const ViewpointAnalyzer&) = delete;

    // Lifecycle
    void init();
    void cleanup();

    // Configuration
    const ViewpointSamplingConfig& config() const
    {
        return _config;
    }
    void setConfig(const ViewpointSamplingConfig& config);

    // Candidate generation and scene analysis
    [[nodiscard]] virtual std::vector<CameraCandidate> generateCandidates(const scene::BoundingBox& bounds) const;
    [[nodiscard]] std::vector<ViewSample> analyze(scene::Node* rootNode);
    [[nodiscard]] std::vector<ViewSample> analyze(scene::Scene* scene);

    // Results and scoring
    [[nodiscard]] const std::vector<AnalyzedSubmesh>& analyzedSubmeshes() const
    {
        return _analyzedSubmeshes;
    }
    [[nodiscard]] static const ViewSample* bestSample(const std::vector<ViewSample>& samples);

  protected:
    [[nodiscard]] virtual float score(const ViewSample& sample) const;

  private:
    struct DrawItem;
    draw::Engine* _engine = nullptr;
    ViewpointSamplingConfig _config;
    std::shared_ptr<gpu::Image> _identifierImage;
    std::shared_ptr<gpu::Image> _depthImage;
    std::shared_ptr<gpu::PipelineLayout> _pipelineLayout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::vector<AnalyzedSubmesh> _analyzedSubmeshes;
    uint32_t _resourceWidth = 0;
    uint32_t _resourceHeight = 0;

    void validateConfig() const;
    void ensureResources();
    void createResources();
};

}
