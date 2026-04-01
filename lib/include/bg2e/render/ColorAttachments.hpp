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

class BG2E_API ColorAttachments {
public:
    ColorAttachments(Engine *, const std::vector<VkFormat>& formats);
    virtual ~ColorAttachments();
    
    void build(VkExtent2D extent);

    std::shared_ptr<vulkan::Image> imageWithIndex(uint32_t index) const;
    
    inline uint32_t size() const { return static_cast<uint32_t>(_images.size()); }
    
    inline const VkExtent2D& extent() const { return _extent; }
    inline const std::vector<VkFormat>& attachmentFormats() const { return _attachmentFormats; }
    inline const std::vector<const vulkan::Image*>& images() const { return _targetImages; }
    
    void cleanup();
    
protected:
    Engine * _engine;
    
    VkExtent2D _extent;
    std::vector<std::shared_ptr<vulkan::Image>> _images;
    
    // Resources and weak pointers
    std::vector<VkFormat> _attachmentFormats;
    std::vector<const vulkan::Image*> _targetImages;
};

}
}
