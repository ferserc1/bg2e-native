//
//  Image.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Texture.hpp>
#include <bg2e/render/MaterialBase.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API TextureWidgets {
public:
    
    inline void setEditTexture(std::shared_ptr<render::Texture> tex)
    {
        clearDS();
        _texture = tex;
        initDS();
    }
    
    inline void clearTexture()
    {
        clearDS();
        _texture.reset();
    }
    
    void drawImage(uint32_t width, uint32_t height, bool sameLine = false);
    
    void selectTexture(const std::string& label, std::function<std::shared_ptr<render::Texture>(base::Texture* tex)>);
    
    void cleanup();

protected:

    std::shared_ptr<render::Texture> _texture;
    
    VkDescriptorSet _textureDS = VK_NULL_HANDLE;
    
    void initDS();
    void clearDS();
};

}
}
