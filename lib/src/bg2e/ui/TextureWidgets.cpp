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

#include <bg2e/ui/TextureWidgets.hpp>
#include <bg2e/app/FileHistory.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/utils/TextureCache.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Button.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace bg2e::ui {

TextureWidgets::TextureWidgets()
    : _filePicker{ app::FileHistory::Image }
{
}

void TextureWidgets::init(render::Engine* engine)
{
    _engine = engine;
    _filePicker.setThumbnailProvider(
        [this](const std::filesystem::path& path) -> FileHistoryWidget::TextureID {
            return thumbnailForPath(path);
        });
}

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
    updateDeferredTexture();

    bool picked = _filePicker.drawImageButton(
        label.starts_with("##") ? "imagePick" + label : "imagePick##" + label,
        reinterpret_cast<FileHistoryWidget::TextureID>(_textureDS), 42, 42);

    if (picked)
    {
        const auto & filePath = _filePicker.selectedPath();
        base::Texture * texture = new base::Texture();
        texture->setImageFilePath(filePath.string());
        texture->setMagFilter(base::Texture::FilterLinear);
        texture->setMinFilter(base::Texture::FilterLinear);
        texture->setUseMipmaps(true);
        auto tex = textureCallback(texture);
        setDeferredTexture(tex);
    }

    if (Button::button(label.starts_with("##") ? "Clear" + label : "Clear##" + label, true)) {
        auto tex = textureCallback(nullptr);
        setDeferredTexture(tex);
    }
    
    if (!label.starts_with("##"))
    {
        Text::text(label, true);
    }
    
    
    return picked;
}

void TextureWidgets::cleanup()
{
    clearDS();
    clearThumbnails();
    _texture.reset();
    _deferredTexture.reset();
}

FileHistoryWidget::TextureID TextureWidgets::thumbnailForPath(const std::filesystem::path& path)
{
    if (_engine == nullptr)
    {
        return 0;
    }

    auto key = path.string();
    auto it = _thumbnailDS.find(key);
    if (it != _thumbnailDS.end())
    {
        return reinterpret_cast<FileHistoryWidget::TextureID>(it->second);
    }

    try
    {
        auto texture = utils::TextureCache::get().load(_engine, path);
        if (!texture)
        {
            return 0;
        }

        auto descriptorSet = ImGui_ImplVulkan_AddTexture(
            texture->sampler(),
            texture->image()->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        _thumbnailDS[key] = descriptorSet;
        return reinterpret_cast<FileHistoryWidget::TextureID>(descriptorSet);
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

void TextureWidgets::clearThumbnails()
{
    if (_thumbnailDS.empty())
    {
        return;
    }

    if (_engine != nullptr)
    {
        _engine->device().waitIdle();
        for (const auto & [path, descriptorSet] : _thumbnailDS)
        {
            ImGui_ImplVulkan_RemoveTexture(descriptorSet);
        }
    }
    _thumbnailDS.clear();
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
