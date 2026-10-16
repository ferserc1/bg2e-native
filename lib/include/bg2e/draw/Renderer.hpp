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

#include <bg2e/draw/environment/all.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/Upscaler.hpp>
#include <bg2e/draw/pipeline/OverlayPass.hpp>
#include <stdexcept>
#include <cstddef>

namespace bg2e {
namespace draw {
namespace pipeline {
class SoftShadowPass;
}

// Describes a resolved single-sample rendering surface, not a window size.
// Resolution policy is implemented later; minimized windows skip rendering.
struct RendererConfiguration {
    gpu::Size2D outputExtent;
    gpu::Size2D internalExtent;
    gpu::PixelFormat outputFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
    gpu::PixelFormat depthFormat = gpu::PixelFormat::D32_SFLOAT;
    std::vector<pipeline::LayerSelection> layers;
    std::shared_ptr<const environment::EnvironmentResources> environment;
    std::shared_ptr<pipeline::SoftShadowPass> softShadows; // Null selects the public default query pass.
    std::shared_ptr<Upscaler> upscaler; // Null selects public bilinear reconstruction.
    std::shared_ptr<ResolutionSettings> resolution; // Null preserves fixed/follow-output policy.
    // Optional ordered post-final passes; shared ownership, one renderer lifecycle each.
    std::vector<std::shared_ptr<pipeline::OverlayPass>> overlays;
};

// Public runtime renderer family. The delegate exclusively owns its renderer
// (unique_ptr in DefaultRenderLoopDelegate); configurations retain injected shared components.
// Implementations own configuration/settings and return their live values.
// initialize -> attachScene -> update/record; resize/replacement/cleanup happen
// at a synchronized boundary. cleanup releases engine/scene/pass references.
// Initialization may be partial on failure: cleanup must tolerate that state.
// resize updates the live configuration and extent-dependent resources; a
// surface format change performs cleanup/initialize on the same renderer owner.
// update owns scene traversal/animation policy (no duplicate delegate visitor).
// sceneFrameNumber/sceneDeltaSeconds describe scene work rather than UI frames.
// Reattaching a scene resets scene-dependent resources. Scene updates remain
// high-level; these headers never include production render scene wrappers.
class BG2E_API Renderer {
  public:
    virtual ~Renderer() = default;
    virtual const RendererConfiguration& configuration() const = 0;
    virtual void initialize(Engine& engine, RendererConfiguration configuration) = 0;
    virtual void attachScene(std::shared_ptr<scene::Node> sceneRoot) = 0;
    virtual void resize(gpu::Size2D outputExtent, gpu::Size2D internalExtent) = 0;
    // Before acquisition; true requests a scene frame. No active command scope.
    virtual bool prepareFrame()
    {
        return false;
    }
    virtual gpu::Size2D resolveInternalExtent(gpu::Size2D output, gpu::Size2D fixed) const
    {
        const auto policy = configuration().resolution;
        return policy ? policy->resolve(output, fixed) : fixed;
    }
    virtual bool needsReconfiguration() const
    {
        return false;
    }
    virtual void update(const FrameContext& frame) = 0;
    virtual void record(const FrameContext& frame) = 0;
    virtual void cleanup() = 0;

    // Structural validation before allocating resources. Device capabilities,
    // actual shader ABI and feature availability also require backend checks.
    static void validateConfiguration(const RendererConfiguration& configuration)
    {
        if (configuration.outputExtent.isZero() || configuration.internalExtent.isZero())
        {
            throw std::invalid_argument("Renderer extents must be nonzero");
        }
        if (configuration.outputFormat == gpu::PixelFormat::Undefined ||
            gpu::isDepthFormat(configuration.outputFormat) ||
            !gpu::isDepthFormat(configuration.depthFormat))
        {
            throw std::invalid_argument("Renderer color/depth formats are incompatible");
        }
        for (size_t i = 0; i < configuration.overlays.size(); ++i)
        {
            if (!configuration.overlays[i])
            {
                throw std::invalid_argument("Renderer overlay must not be null");
            }
            for (size_t j = 0; j < i; ++j)
            {
                if (configuration.overlays[i] == configuration.overlays[j])
                {
                    throw std::invalid_argument("Renderer overlay cannot occupy two lifecycle slots");
                }
            }
        }
        for (const auto& selection : configuration.layers)
        {
            if (!selection.layer || selection.configuration.outputExtent != configuration.outputExtent ||
                selection.configuration.internalExtent != configuration.internalExtent)
            {
                throw std::invalid_argument("Renderer layer or extents are incompatible");
            }
            validateContract(selection.layer->contract(), selection.configuration.contract);
        }
    }

    // Shared dispatch path for application-owned compatible passes. Derived
    // renderers resolve their graph inputs/outputs, then call this helper;
    // validation precedes the injected object's virtual recording operation.
    static void recordLayer(const pipeline::LayerSelection& selection,
                            const pipeline::PassContext& context)
    {
        if (!selection.layer || &context.configuration != &selection.configuration)
        {
            throw std::invalid_argument("Pass context must use the selected configuration");
        }
        validateContract(selection.layer->contract(), context.configuration.contract);
        if (context.frame.extent != context.configuration.outputExtent)
        {
            throw std::invalid_argument("Frame output extent is incompatible");
        }
        validateBindings(context.configuration.contract.inputs, context.inputs, context);
        validateBindings(context.configuration.contract.outputs, context.outputs, context);
        selection.layer->record(context);
    }

  private:
    static void validateContract(const pipeline::PassContract& actual, const pipeline::PassContract& expected)
    {
        if (expected.shaderInterface.empty() || actual.shaderInterface != expected.shaderInterface)
        {
            throw std::invalid_argument("Pass shader interface is incompatible");
        }
        validateSlots(actual.inputs, expected.inputs);
        validateSlots(actual.outputs, expected.outputs);
    }

    static void validateSlots(const std::vector<pipeline::ImageSlot>& actual,
                              const std::vector<pipeline::ImageSlot>& expected)
    {
        if (actual.size() != expected.size())
        {
            throw std::invalid_argument("Pass slot count is incompatible");
        }
        for (size_t i = 0; i < expected.size(); ++i)
        {
            const auto& slot = expected[i];
            if (slot.semantic.empty() || slot.format == gpu::PixelFormat::Undefined ||
                actual[i].semantic != slot.semantic || actual[i].format != slot.format ||
                actual[i].extent != slot.extent || actual[i].usage != slot.usage ||
                actual[i].type != slot.type)
            {
                throw std::invalid_argument("Pass image contract is incompatible");
            }
            for (size_t j = 0; j < i; ++j)
            {
                if (expected[j].semantic == slot.semantic)
                {
                    throw std::invalid_argument("Duplicate pass image semantic");
                }
            }
        }
    }

    static void validateBindings(const std::vector<pipeline::ImageSlot>& slots,
                                 const std::vector<pipeline::ImageBinding>& bindings,
                                 const pipeline::PassContext& context)
    {
        if (slots.size() != bindings.size())
        {
            throw std::invalid_argument("Pass image binding count is incompatible");
        }
        for (size_t i = 0; i < slots.size(); ++i)
        {
            const auto* image = bindings[i].image;
            const auto extent = slots[i].extent == pipeline::ExtentDomain::Output
                ? context.configuration.outputExtent : context.configuration.internalExtent;
            if (bindings[i].semantic != slots[i].semantic || !image || !image->isValid() ||
                image->imageType() != slots[i].type || image->size() != extent ||
                image->pixelFormat() != slots[i].format ||
                (image->usage() & slots[i].usage) != slots[i].usage ||
                image->device() != context.frame.engine.device())
            {
                throw std::invalid_argument("Pass image binding is incompatible");
            }
        }
    }
};

}
}
