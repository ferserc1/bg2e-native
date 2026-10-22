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
#include <bg2e/draw/SettingsCatalog.hpp>
#include <bg2e/draw/pipeline/FullscreenPass.hpp>
#include <bg2e/math/base.hpp>
#include <memory>

namespace bg2e::draw {
// All calls run on the render thread. Initialize/resize/cleanup require a drained
// device. record borrows inputs; implementations retain descriptors per acquired
// slot until completion. Output is single-sample linear HDR, before tone mapping.
struct UpscalerConfiguration {
    gpu::Size2D internalExtent, outputExtent;
    uint32_t frameSlots = 0;
    std::filesystem::path shaderRoot;
};
struct UpscalerCamera {
    glm::mat4 view{1}, projection{1}; // Unjittered, Vulkan/Metal zero-to-one depth.
    float nearPlane = 0.1f, farPlane = 1000.0f, verticalFov = 1.04719755f;
};
// Full-extent opaque receiver metadata, never the last transparent layer.
// Borrowed pointers must survive GPU completion; supplied matching owners are
// retained by consumers per acquired slot. Motion is previous-current unjittered
// framebuffer pixels at internal extent. Token zero selects camera/depth fallback.
struct UpscalerOpaqueMotion {
    gpu::Image* motion = nullptr;
    gpu::Image* token = nullptr;
    std::shared_ptr<gpu::Image> motionOwner, tokenOwner;

    void validate(const gpu::Device& device, gpu::Size2D internalExtent) const;
};
struct UpscalerInput {
    const FrameContext& frame;
    gpu::Image& color;
    gpu::Image& opaqueDepth; // Preserved opaque depth, never last glass depth.
    gpu::Image& output;
    UpscalerCamera camera;
    glm::mat4 jitteredProjection{1};
    UpscalerOpaqueMotion opaqueMotion; // Optional trailing pair; existing aggregate callers remain valid.
};
struct UpscalerRequirements {
    bool opaqueDepth = false;
    bool cameraMotion = false;
    bool jitter = false;
    bool storageOutput = false;
};
class BG2E_API Upscaler {
  public:
    virtual ~Upscaler() = default;
    virtual std::string identifier() const = 0;
    virtual std::string label() const = 0;
    virtual UpscalerRequirements requirements() const = 0;
    virtual void initialize(Engine&, const UpscalerConfiguration&) = 0;
    virtual void resize(const UpscalerConfiguration&) = 0;
    virtual bool available() const = 0;
    virtual std::string status() const = 0;
    virtual glm::mat4 prepare(const UpscalerCamera& camera, uint64_t sceneFrameNumber)
    {
        return camera.projection;
    }
    // settingsType accessors receive the complete object address (dynamic_cast<void*>),
    // allowing application implementations with multiple inheritance.
    // False means no usable result: caller reconstructs unmodified HDR instead.
    // Never throw merely because the optional SDK/backend is unavailable.
    virtual bool record(const UpscalerInput&) = 0;
    virtual void reset() = 0;
    virtual void cleanup() = 0;
    // Metadata refers to the live implementation, with stable property IDs.
    // The renderer retains this object; the catalog also retains its owner.
    virtual reflection::TypeInfo settingsType() const = 0;
    virtual bool needsRebuild() const
    {
        return false;
    }
};
class BG2E_API ResolutionSettings {
  public:
    float scale() const;
    void setScale(float value);
    bool enabled() const;
    void setEnabled(bool value);
    gpu::Size2D resolve(gpu::Size2D output, gpu::Size2D fixed) const;
    reflection::TypeInfo settingsType() const;
  private:
    float _scale = 1.0f;
    bool _enabled = false; // Preserve explicit fixed extents unless opted into scaling.
};
class BG2E_API ResamplingUpscaler : public Upscaler {
  public:
    std::string identifier() const override;
    std::string label() const override;
    UpscalerRequirements requirements() const override;
    void initialize(Engine&, const UpscalerConfiguration&) override;
    void resize(const UpscalerConfiguration&) override;
    bool available() const override;
    std::string status() const override;
    bool record(const UpscalerInput&) override;
    void reset() override;
    void cleanup() override;
    reflection::TypeInfo settingsType() const override;
  private:
    bool _ready = false;
    pipeline::FullscreenPass _pass;
};
}
