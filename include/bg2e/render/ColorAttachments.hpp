//
//  ColorAttachments.hpp

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
    
    void build(VkExtent2D extent, VkSampleCountFlagBits samples);

    std::shared_ptr<vulkan::Image> imageWithIndex(uint32_t index) const;
    
    inline uint32_t size() const { return static_cast<uint32_t>(_images.size()); }
    
    inline const VkExtent2D& extent() const { return _extent; }
    inline const std::vector<VkFormat>& attachmentFormats() const { return _attachmentFormats; }
    inline const std::vector<const vulkan::Image*>& images() const { return _targetImages; }
    
    void cleanup();
    
protected:
    Engine * _engine;
    
    VkExtent2D _extent;
    VkSampleCountFlagBits _samples;
    std::vector<std::shared_ptr<vulkan::Image>> _images;
    
    // Resources and weak pointers
    std::vector<VkFormat> _attachmentFormats;
    std::vector<const vulkan::Image*> _targetImages;
};

}
}
