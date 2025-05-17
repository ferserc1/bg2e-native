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
    Texture(Vulkan* vulkan, std::shared_ptr<base::Texture> texture, std::shared_ptr<vulkan::Image> image) : _vulkan(vulkan) { load(texture, image); }
    Texture(Vulkan* vulkan, base::Texture* texture, std::shared_ptr<vulkan::Image> image) : _vulkan(vulkan) { load(texture, image); }
    Texture(Vulkan* vulkan, base::Texture* texture, vulkan::Image* image) : _vulkan(vulkan) { load(texture, image); }
    Texture(Vulkan* vulkan, std::shared_ptr<base::Texture> texture, vulkan::Image* image) : _vulkan(vulkan) { load(texture, image); }
    Texture(Vulkan* vulkan, std::shared_ptr<vulkan::Image> image) : _vulkan(vulkan) { load(image); }
    Texture(Vulkan* vulkan, vulkan::Image* image) : _vulkan(vulkan) { load(image); }
    ~Texture();

	inline void load(base::Texture* texture) { load(std::shared_ptr<base::Texture>(texture)); }
    void load(std::shared_ptr<base::Texture> texture);
    
    // Create a texture from a Vulkan image: the vulkan::Image is used as image data, and the
    // base::Texture is used to create the sampler
    inline void load(base::Texture* texture, vulkan::Image* image)
    {
        load(std::shared_ptr<base::Texture>(texture), std::shared_ptr<vulkan::Image>(image));
    }
    
    inline void load(base::Texture* texture, std::shared_ptr<vulkan::Image> image)
    {
        load(std::shared_ptr<base::Texture>(texture), image);
    }
    
    inline void load(std::shared_ptr<base::Texture> texture, vulkan::Image* image)
    {
        load(texture, std::shared_ptr<vulkan::Image>(image));
    }
    
    void load(std::shared_ptr<base::Texture> texture, std::shared_ptr<vulkan::Image> image);
    
    inline void load(std::shared_ptr<vulkan::Image> image)
    {
        load(new base::Texture(), image);
    }
    
    inline void load(vulkan::Image* image)
    {
        load(new base::Texture(), image);
    }
    
    inline base::Texture* base() { return _texture.get(); }
    inline const base::Texture* base() const { return _texture.get(); }
    inline vulkan::Image* image() { return _image.get(); }
    inline const vulkan::Image* image() const { return _image.get(); }
    inline VkSampler sampler() const { return _sampler; }

protected:
    Vulkan * _vulkan;
    std::shared_ptr<base::Texture> _texture;
    std::shared_ptr<vulkan::Image> _image;
    bool _hasImageOwnership = false;

    VkSampler _sampler = VK_NULL_HANDLE;
    
    // Called by destructor
    void cleanup();
};

}
}
