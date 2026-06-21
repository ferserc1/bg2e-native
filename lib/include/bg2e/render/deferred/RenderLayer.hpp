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
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/EnvironmentResources.hpp>
#include <bg2e/scene/Scene.hpp>

#include <glm/glm.hpp>

namespace bg2e {
namespace render {
namespace deferred {

class BG2E_API RenderLayer {
public:
    RenderLayer(Engine* engine);
    virtual ~RenderLayer();

    virtual void build(VkExtent2D extent, VkFormat outputFormat);
    virtual void initFrameResources(vulkan::DescriptorSetAllocator* allocator);
    virtual void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vulkan::Image* inputImage,
        const vulkan::Image* outputImage,
        vulkan::FrameResources& frameResources
    );
    virtual void resize(VkExtent2D newExtent);
    virtual void cleanup();

    void setScene(scene::Scene* scene) { _scene = scene; }
    void setEnvironment(EnvironmentResources* environment) { _environment = environment; }

    // Pass a non-null pointer to override the camera projection matrix used
    // during rendering (e.g. with FSR jitter applied).  nullptr restores the
    // default behaviour of reading from the scene camera.
    void setProjectionOverride(const glm::mat4* proj) { _projectionOverride = proj; }

    void setColorCorrection(
        float brightness,
        float contrast,
        float exposure
    ) {
        _brightness = brightness;
        _contrast = contrast;
        _exposure = exposure;
    }

protected:
    Engine* _engine;
    VkExtent2D _extent;
    VkFormat _outputFormat;
    scene::Scene* _scene = nullptr;
    EnvironmentResources* _environment = nullptr;
    float _brightness = 0.0f;
    float _contrast = 1.0f;
    float _exposure = 1.0f;

    const glm::mat4* _projectionOverride = nullptr;
};

}
}
}
