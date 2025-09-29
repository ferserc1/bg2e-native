//
//  Image.cpp

#include <bg2e/ui/TextureWidgets.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace bg2e::ui {

void TextureWidgets::drawImage(uint32_t width, uint32_t height, bool sameLine)
{
    if (_textureDS != VK_NULL_HANDLE)
    {
        if (sameLine)
        {
            ImGui::SameLine();
        }
        ImGui::Image(static_cast<ImTextureID>(_textureDS), ImVec2(static_cast<float>(width), static_cast<float>(height)));
    }
}

void TextureWidgets::cleanup()
{
    clearDS();
    _texture.reset();
}
    
void TextureWidgets::initDS()
{
    _textureDS = ImGui_ImplVulkan_AddTexture(
        _texture->sampler(),
        _texture->image()->imageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void TextureWidgets::clearDS()
{
    if (_textureDS != VK_NULL_HANDLE)
    {
        ImGui_ImplVulkan_RemoveTexture(_textureDS);
        _textureDS = VK_NULL_HANDLE;
    }
}
    
}
