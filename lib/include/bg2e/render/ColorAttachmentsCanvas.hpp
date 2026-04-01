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
#include <bg2e/render/ColorAttachments.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>

#include <memory>
#include <string>

namespace bg2e {
namespace render {

class BG2E_API ColorAttachmentsCanvas {
public:
    ColorAttachmentsCanvas(Engine *, std::shared_ptr<ColorAttachments> attachments);
    
    // TODO: Allow to add uniform buffers
    
    void initFrameResources(vulkan::DescriptorSetAllocator*);
    
    void build(
        const std::string& fragmentShader,
        VkFormat targetImageFormat,
        VkSampleCountFlagBits sampleCount
    );
    
    void draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vulkan::Image * targetImage,
        vulkan::FrameResources& frameResources
    );
    
protected:
    Engine * _engine;
    
    std::shared_ptr<ColorAttachments> _attachments;
    
    VkPipeline _pipeline;
    VkPipelineLayout _pipelineLayout;
    VkDescriptorSetLayout _attachmentsDSLayout;
    VkSampler _attSampler;
};

}
}
