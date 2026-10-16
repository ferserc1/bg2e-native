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

#include <bg2e/ui/DrawTextureWidgets.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/ui/FileHistoryWidget.hpp>
#include <bg2e/app/FileHistory.hpp>
#include <bg2e/draw/Texture.hpp>
#include <imgui.h>
#include <exception>

namespace bg2e::ui {
struct DrawTextureWidgets::Impl {
    FileHistoryWidget picker{app::FileHistory::Image};
    std::shared_ptr<TexturePreview> preview;
    std::weak_ptr<gpu::Image> image;
    std::weak_ptr<gpu::Sampler> sampler;
    UserInterface* ui = nullptr;
};

DrawTextureWidgets::DrawTextureWidgets() : _impl(std::make_unique<Impl>())
{
}

DrawTextureWidgets::~DrawTextureWidgets() = default;

DrawTextureWidgets::Result DrawTextureWidgets::draw(UserInterface& ui, const std::string& label, const std::shared_ptr<draw::Texture>& texture)
{
    ImGui::PushID(this);
    Result result;
    try
    {
        if (!texture)
        {
            cleanup();
        }
        else if (!_impl->preview || !_impl->preview->valid() || _impl->ui != &ui ||
            _impl->image.lock().get() != texture->image() || _impl->sampler.lock().get() != texture->sampler())
        {
            _impl->preview = ui.texturePreview(texture);
            _impl->image = texture->imageResource();
            _impl->sampler = texture->samplerResource();
            _impl->ui = &ui;
        }
        if (_impl->preview)
        {
            _impl->preview->drawImage(64, 64);
        }
    }
    catch (const std::exception& error)
    {
        cleanup();
        ImGui::TextWrapped("%s", error.what());
    }
    result.selected = _impl->picker.draw(label, "Select texture...");
    if (result.selected)
    {
        result.path = _impl->picker.selectedPath();
    }
    ImGui::SameLine();
    result.cleared = ImGui::Button("Clear");
    ImGui::PopID();
    return result;
}

void DrawTextureWidgets::cleanup()
{
    _impl->preview.reset();
    _impl->image.reset();
    _impl->sampler.reset();
    _impl->ui = nullptr;
}

}
