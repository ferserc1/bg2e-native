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

#include <bg2e/ui/TexturePreview.hpp>
#include "detail/PreviewRegistry.hpp"
#include <bg2e/gpu/Device.hpp>
#include <imgui.h>
#include <algorithm>
#include <stdexcept>

namespace bg2e::ui::detail {

PreviewRegistry::PreviewRegistry(draw::Engine& engine, ImGuiBackend& backend)
    : _context(ImGui::GetCurrentContext()), _engine(&engine), _backend(&backend)
{
}

std::shared_ptr<TexturePreview> PreviewRegistry::create(const std::shared_ptr<draw::Texture>& texture)
{
    if (!_backend || !texture || texture->engine() != _engine || !texture->image() || !texture->sampler() ||
        !texture->image()->isValid() || !texture->sampler()->isValid() ||
        texture->image()->imageType() != gpu::ImageType::Image2D || gpu::isDepthFormat(texture->image()->pixelFormat()) ||
        gpu::isUnsignedIntegerFormat(texture->image()->pixelFormat()) || gpu::isSignedIntegerFormat(texture->image()->pixelFormat()) ||
        !gpu::hasFlag(texture->image()->usage(), gpu::ImageUsage::Sampled) ||
        texture->image()->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
    {
        throw std::invalid_argument("Preview needs a sampleable color texture from this draw UI engine");
    }
    auto entry = std::make_shared<PreviewEntry>();
    entry->registry = shared_from_this();
    entry->image = texture->imageResource();
    entry->sampler = texture->samplerResource();
    _entries.push_back(entry);
    return std::shared_ptr<TexturePreview>(new TexturePreview(std::move(entry)));
}

uint64_t PreviewRegistry::identifier(PreviewEntry& entry)
{
    if (!_backend || !entry.image || !entry.sampler)
    {
        return 0;
    }
    if (ImGui::GetCurrentContext() != _context)
    {
        throw std::logic_error("Preview belongs to another UI context");
    }
    const auto generation = _backend->textureGeneration();
    if (!entry.identifier || entry.generation != generation)
    {
        entry.identifier = _backend->registerTexture(*entry.image, *entry.sampler);
        entry.generation = generation;
    }
    return entry.identifier;
}

void PreviewRegistry::beginFrame()
{
    if (!_backend)
    {
        return;
    }
    // The registry is the last owner only after all widget handles are gone.
    // Drain at the following UI frame, never while its draw data is being built.
    if (std::any_of(_entries.begin(), _entries.end(), [](const auto& entry)
    {
        return entry.use_count() == 1;
    }))
    {
        _engine->device()->waitIdle();
        std::erase_if(_entries, [&](const auto& entry)
        {
            if (entry.use_count() != 1)
            {
                return false;
            }
            if (entry->identifier && entry->generation == _backend->textureGeneration())
            {
                _backend->removeTexture(entry->identifier);
            }
            return true;
        });
    }
}

void PreviewRegistry::shutdown()
{
    if (!_backend)
    {
        return;
    }
    _engine->device()->waitIdle();
    for (auto& entry : _entries)
    {
        if (entry->identifier && entry->generation == _backend->textureGeneration())
        {
            _backend->removeTexture(entry->identifier);
        }
        entry->identifier = 0;
        entry->image.reset();
        entry->sampler.reset();
    }
    _entries.clear();
    _backend = nullptr;
    _engine = nullptr;
}

}

namespace bg2e::ui {

TexturePreview::TexturePreview(std::shared_ptr<detail::PreviewEntry> entry) : _entry(std::move(entry))
{
}

bool TexturePreview::valid() const
{
    return _entry && !_entry->registry.expired() && _entry->image && _entry->sampler;
}

void TexturePreview::drawImage(uint32_t width, uint32_t height) const
{
    if (valid())
    {
        if (auto registry = _entry->registry.lock())
        {
            ImGui::Image(registry->identifier(*_entry), ImVec2(float(width), float(height)));
        }
    }
}

bool TexturePreview::imageButton(const std::string& id, uint32_t width, uint32_t height) const
{
    if (valid())
    {
        if (auto registry = _entry->registry.lock())
        {
            return ImGui::ImageButton(id.c_str(), registry->identifier(*_entry), ImVec2(float(width), float(height)));
        }
    }
    return false;
}

}
