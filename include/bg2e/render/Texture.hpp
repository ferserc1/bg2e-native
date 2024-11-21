#pragma once

#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/base/Texture.hpp>

namespace bg2e {
namespace render {

class BG2E_API Texture {
public:
    Texture(Vulkan * vulkan) : _vulkan(vulkan) {}
	Texture(Vulkan* vulkan, std::shared_ptr<base::Texture> texture) : _vulkan(vulkan) { load(texture); }
	Texture(Vulkan* vulkan, base::Texture* texture) : _vulkan(vulkan) { load(std::shared_ptr<base::Texture>(texture)); }

	inline void load(base::Texture* texture) { load(std::shared_ptr<base::Texture>(texture)); }
    void load(std::shared_ptr<base::Texture> texture);

    void cleanup();
    
    inline base::Texture* base() { return _texture.get(); }
    inline const base::Texture* base() const { return _texture.get(); }
    inline vulkan::Image* image() { return _image.get(); }
    inline const vulkan::Image* image() const { return _image.get(); }
    inline VkSampler sampler() const { return _sampler; }

protected:
    Vulkan * _vulkan;
    std::shared_ptr<base::Texture> _texture;
    std::shared_ptr<vulkan::Image> _image;

    VkSampler _sampler = VK_NULL_HANDLE;
};

}
}
