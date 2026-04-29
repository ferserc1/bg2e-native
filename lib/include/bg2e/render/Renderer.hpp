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

#include <memory>
#include <vulkan/vulkan.h>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void build(
        bg2e::render::Engine* engine,
        VkExtent2D initialExtent,
        VkFormat colorImageFormat,
        VkFormat depthImageFormat,
        VkSampleCountFlagBits sampleCount,
        bool isOffscreen
    ) = 0;
    
    virtual void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) = 0;
    virtual void initScene(std::shared_ptr<bg2e::scene::Node> sceneRoot) = 0;
    virtual void resize(VkExtent2D newExtent) = 0;
    virtual void update(float delta) = 0;
    virtual void draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* colorImage,
        const bg2e::render::vulkan::Image* depthImage,
        const bg2e::render::vulkan::Image* msaaDepthImage,
        bg2e::render::vulkan::FrameResources& frameResources
    ) = 0;
    virtual void cleanup() = 0;

    virtual bg2e::scene::Scene* scene() = 0;
    
    virtual void setBrightness(float b) = 0;
    virtual void setContrast(float c) = 0;
    virtual void setExposure(float e) = 0;
    virtual float brightness() const = 0;
    virtual float contrast() const = 0;
    virtual float exposure() const = 0;
    
    virtual uint32_t viewportWidth() = 0;
    virtual uint32_t viewportHeight() = 0;
};

}
}
