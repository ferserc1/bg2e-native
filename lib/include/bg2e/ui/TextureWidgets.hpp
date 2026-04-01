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
    
    // If the texture is to be changed during the same frame in which it is being rendered, this function must be used so that
    // it is not changed until the next frame.
    inline void setDeferredTexture(std::shared_ptr<render::Texture> tex)
    {
        _deferredTexture = tex;
    }
    
    inline void clearTexture()
    {
        clearDS();
        _texture.reset();
    }
    
    void drawImage(uint32_t width, uint32_t height, bool sameLine = false);
    
    bool imageButton(const std::string& id, uint32_t width, uint32_t height, bool sameLine = false);
    
    bool selectTexture(const std::string& label, std::function<std::shared_ptr<render::Texture>(base::Texture* tex)>);
    
    void cleanup();

protected:

    std::shared_ptr<render::Texture> _texture;
    std::shared_ptr<render::Texture> _deferredTexture;
    
    VkDescriptorSet _textureDS = VK_NULL_HANDLE;
    
    void initDS();
    void clearDS();
    void updateDeferredTexture();
};

}
}
