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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {

class BG2E_API GBufferManager {
public:
    GBufferManager(Engine * engine);
    ~GBufferManager();

    void build(VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void cleanup();

    uint32_t imageCount() const;
    std::shared_ptr<vulkan::Image> image(uint32_t index) const;
    std::shared_ptr<vulkan::Image> depthImage() const;
    const std::vector<const vulkan::Image*>& images() const;
    const std::vector<VkFormat>& formats() const;
    VkFormat depthFormat() const;
    VkExtent2D extent() const;

    void transitionToClear(VkCommandBuffer cmd);
    void transitionToAttachment(VkCommandBuffer cmd);
    void transitionToShaderRead(VkCommandBuffer cmd);
    void beginRender(VkCommandBuffer cmd);

private:
    Engine * _engine;
    
    VkExtent2D _extent;
    
    std::vector<std::shared_ptr<vulkan::Image>> _colorImages;
    std::vector<const vulkan::Image*> _colorImagePtrs;
    std::vector<VkFormat> _colorFormats;
    
    std::shared_ptr<vulkan::Image> _depthImage;
    VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageLayout _colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout _depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    void transitionTo(VkCommandBuffer cmd, VkImageLayout colorLayout, VkImageLayout depthLayout);
};

}
}
