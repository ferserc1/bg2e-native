//
//  Image.cpp

#include <bg2e/ui/TextureWidgets.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/app/FileDialog.hpp>
#include <bg2e/render/Engine.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace bg2e::ui {

void TextureWidgets::drawImage(uint32_t width, uint32_t height, bool sameLine)
{
    updateDeferredTexture();
    if (_textureDS != VK_NULL_HANDLE)
    {
        if (sameLine)
        {
            ImGui::SameLine();
        }
        ImGui::Image(
            reinterpret_cast<ImTextureID>(_textureDS),
            ImVec2(static_cast<float>(width), static_cast<float>(height))
        );
    }
}

bool TextureWidgets::imageButton(const std::string& id, uint32_t width, uint32_t height, bool sameLine)
{
    updateDeferredTexture();
    if (_textureDS != VK_NULL_HANDLE)
    {
        if (sameLine)
        {
            ImGui::SameLine();
        }
        if (ImGui::ImageButton(
                id.c_str(),
                reinterpret_cast<ImTextureID>(_textureDS),
                ImVec2(static_cast<float>(width), static_cast<float>(height)),
                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
            )
        ) {
            return true;
        }
    }
    return false;
}

bool TextureWidgets::selectTexture(const std::string& label, std::function<std::shared_ptr<render::Texture>(base::Texture* tex)> textureCallback)
{
    std::filesystem::path filePath;
    
    if (imageButton(label.starts_with("##") ? "imagePick" + label : "imagePick##" + label, 42, 42)) {
        filePath = app::FileDialog::getOpenFilePath(app::FileDialog::imageFilters);
        if (!filePath.empty())
        {
            base::Texture * texture = new base::Texture();
            texture->setImageFilePath(filePath.string());
            texture->setMagFilter(base::Texture::FilterLinear);
            texture->setMinFilter(base::Texture::FilterLinear);
            texture->setUseMipmaps(true);
            auto tex = textureCallback(texture);
            setDeferredTexture(tex);
        }
    }
    if (BasicWidgets::button(label.starts_with("##") ? "Clear" + label : "Clear##" + label, true)) {
        auto tex = textureCallback(nullptr);
        setDeferredTexture(tex);
    }
    
    if (!label.starts_with("##"))
    {
        BasicWidgets::text(label, true);
    }
    
    
    return !filePath.empty();
}

void TextureWidgets::cleanup()
{
    clearDS();
    _texture.reset();
    _deferredTexture.reset();
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
        // Wait until command queues are completed to release Vulkan resources
        _texture->engine()->device().waitIdle();
        
        ImGui_ImplVulkan_RemoveTexture(_textureDS);
        _textureDS = VK_NULL_HANDLE;
    }
}

void TextureWidgets::updateDeferredTexture()
{
    if (_deferredTexture.get())
    {
        setEditTexture(_deferredTexture);
        _deferredTexture.reset();
    }
}
    
}
