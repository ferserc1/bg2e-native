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

#include <bg2e/draw/FrameContext.hpp>
#include <memory>
#include <string>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
}
namespace draw {
namespace pipeline {

struct RasterFrame;

// All images are single sample. Semantic names and shaderInterface identify
// the producer/consumer ABI, including packing, resource bindings and stages.
// Matching formats alone does not establish shader compatibility.
enum class ExtentDomain
{
    Internal,
    Output,
    Scaled // Explicit producer-defined nonzero extent; normalized sampled inputs.
};

struct ImageSlot {
    std::string semantic;
    gpu::PixelFormat format = gpu::PixelFormat::Undefined;
    ExtentDomain extent = ExtentDomain::Internal;
    gpu::ImageUsage usage = gpu::ImageUsage::Sampled;
    gpu::ImageType type = gpu::ImageType::Image2D;
};

struct PassContract {
    std::string shaderInterface;
    std::vector<ImageSlot> inputs;
    std::vector<ImageSlot> outputs;
};

struct LayerConfiguration {
    gpu::Size2D outputExtent;
    gpu::Size2D internalExtent;
    PassContract contract;
};

struct ImageBinding {
    std::string semantic;
    gpu::Image* image = nullptr;
};

// Borrowed for this recording call only. Images and their GPU resources must
// remain alive until the submitted work completes. The caller supplies correct
// layouts/dependencies; a pass must document transitions in its implementation.
struct PassContext {
    const FrameContext& frame;
    const LayerConfiguration& configuration;
    const std::vector<ImageBinding>& inputs;
    const std::vector<ImageBinding>& outputs;
    scene::Node* sceneRoot = nullptr;
    const RasterFrame* raster = nullptr; // Borrowed draw snapshot/bindings for raster layers.
};

// Public base for G-buffer, lighting, shadow and final-processing passes.
// Configuration and mutable settings live in the concrete pass. initialize,
// resize and cleanup run only after outstanding users have completed. A pass
// instance belongs to one engine/renderer lifecycle, even when shared-owned.
class BG2E_API RenderLayer {
  public:
    virtual ~RenderLayer() = default;
    virtual const PassContract& contract() const = 0;
    virtual void initialize(Engine& engine, const LayerConfiguration& configuration) = 0;
    virtual void attachScene(std::shared_ptr<scene::Node> sceneRoot) = 0;
    virtual void resize(const LayerConfiguration& configuration) = 0;
    virtual void update(const FrameContext& frame) = 0;
    virtual void record(const PassContext& context) = 0;
    virtual void cleanup() = 0;
};

// Retained shared ownership: unique ownership can be transferred into shared_ptr.
// No implicit borrowed ownership or no-op deleter convention. Resources in a
// PassContext are borrowed; lifecycle operations remain the renderer's job.
struct LayerSelection {
    std::shared_ptr<RenderLayer> layer;
    LayerConfiguration configuration;
};

}
}
}
