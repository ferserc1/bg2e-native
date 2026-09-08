/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace bg2e::scene {
class BoundingBox;
class DrawableComponent;
class Node;
class Scene;
}

namespace bg2e::render {

class Engine;

namespace vulkan {
class Image;
}

/** A uniformly sampled scalar interval. Cyclic intervals exclude max. */
struct ViewpointSampleRange
{
    float min = 0.0f;
    float max = 0.0f;
    uint32_t samples = 1;
    bool cyclic = false;
};

struct ViewpointSamplingConfig
{
    uint32_t width = 64;
    uint32_t height = 64;

    // Angles are expressed in degrees. Pitch zero is horizontal and positive
    // pitch places the camera above the target.
    ViewpointSampleRange yaw { 0.0f, 360.0f, 18, true };
    ViewpointSampleRange pitch { 5.0f, 35.0f, 4, false };

    // Fraction from AABB min.y to max.y. Applications may use 0.75 as a preset
    // for some product categories; the engine default remains the AABB center.
    ViewpointSampleRange targetHeight { 0.5f, 0.5f, 1, false };

    // Multiplicative margin over the distance required to fit the AABB sphere.
    ViewpointSampleRange distanceScale { 1.1f, 1.1f, 1, false };
    ViewpointSampleRange fieldOfView { 35.0f, 35.0f, 1, false };
};

struct CameraCandidate
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    float targetHeight = 0.5f;
    float distanceScale = 1.1f;
    float fieldOfView = 35.0f;
    float distance = 0.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    glm::vec3 position { 0.0f };
    glm::vec3 target { 0.0f };
    glm::mat4 viewMatrix { 1.0f };
    glm::mat4 projectionMatrix { 1.0f };
};

struct ViewAnalysis
{
    uint32_t visiblePixelCount = 0;
    uint32_t borderPixelCount = 0;
    uint32_t visibleSubmeshCount = 0;

    glm::uvec2 minPixel { 0, 0 };
    glm::uvec2 maxPixel { 0, 0 };

    // Linear camera-space distances, computed from the sampled depth buffer.
    float minDepth = 0.0f;
    float maxDepth = 0.0f;
    float meanDepth = 0.0f;
    float depthVariance = 0.0f;

    // Indexes match ViewpointAnalyzer::analyzedSubmeshes().
    std::vector<uint32_t> pixelsPerSubmesh;
};

struct ViewSample
{
    CameraCandidate camera;
    ViewAnalysis analysis;
    float score = 0.0f;
};

struct AnalyzedSubmesh
{
    uint32_t identifier = 0;
    std::weak_ptr<scene::Node> node;
    std::weak_ptr<scene::DrawableComponent> drawable;
    uint32_t submesh = 0;
};

/**
 * Synchronously renders and analyzes a configurable batch of candidate views.
 *
 * The analyzer uses the engine's existing RGBA8 picking shaders and a D32 depth
 * target. All candidates are recorded in one immediate command submission and
 * copied to CPU-visible buffers before scoring. The engine must outlive this
 * object and all analyzed drawables must already be loaded on that engine.
 */
class BG2E_API ViewpointAnalyzer
{
public:
    explicit ViewpointAnalyzer(Engine * engine);
    virtual ~ViewpointAnalyzer();

    ViewpointAnalyzer(const ViewpointAnalyzer&) = delete;
    ViewpointAnalyzer& operator=(const ViewpointAnalyzer&) = delete;

    void init();
    void cleanup();

    [[nodiscard]] const ViewpointSamplingConfig& config() const { return _config; }
    void setConfig(const ViewpointSamplingConfig& config);

    [[nodiscard]] virtual std::vector<CameraCandidate> generateCandidates(
        const scene::BoundingBox& bounds
    ) const;

    [[nodiscard]] std::vector<ViewSample> analyze(scene::Node * rootNode);
    [[nodiscard]] std::vector<ViewSample> analyze(scene::Scene * scene);

    [[nodiscard]] const std::vector<AnalyzedSubmesh>& analyzedSubmeshes() const
    {
        return _analyzedSubmeshes;
    }

    [[nodiscard]] static const ViewSample * bestSample(const std::vector<ViewSample>& samples);

protected:
    /** Default policy rewards coverage and penalizes geometry clipped at edges. */
    [[nodiscard]] virtual float score(const ViewSample& sample) const;

private:
    struct DrawItem;
    struct PushConstantData
    {
        glm::mat4 mvp { 1.0f };
        uint32_t identifier = 0;
    };

    Engine * _engine = nullptr;
    ViewpointSamplingConfig _config;

    std::shared_ptr<vulkan::Image> _identifierImage;
    std::shared_ptr<vulkan::Image> _depthImage;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkExtent2D _extent { 0, 0 };

    std::vector<AnalyzedSubmesh> _analyzedSubmeshes;

    void validateConfig() const;
    void ensureResources();
    void createImages();
    void createPipeline();
    void cleanupImages();
    void collectDrawItems(scene::Node * node, std::vector<DrawItem>& items);
};

}
