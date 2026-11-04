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

#include "ImageRegistry.hpp"
#include "ImGuiBackend.hpp"
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <imgui.h>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bg2e::ui::detail {

ImageRegistry::ImageRegistry(draw::Engine& engine, ImGuiBackend& backend)
    : _context(ImGui::GetCurrentContext()), _engine(&engine), _backend(&backend)
{
}

std::shared_ptr<ImageEntry> ImageRegistry::create(
    std::shared_ptr<gpu::Image> image, std::shared_ptr<gpu::Sampler> sampler)
{
    if (!_backend || !image || !sampler || image->device() != _engine->device() ||
        sampler->device() != _engine->device() || !image->isValid() || !sampler->isValid() ||
        image->imageType() != gpu::ImageType::Image2D || gpu::isDepthFormat(image->pixelFormat()) ||
        gpu::isUnsignedIntegerFormat(image->pixelFormat()) || gpu::isSignedIntegerFormat(image->pixelFormat()) ||
        !gpu::hasFlag(image->usage(), gpu::ImageUsage::Sampled) ||
        image->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
    {
        throw std::invalid_argument("UI image needs sampleable color resources from this UI device");
    }
    auto entry = std::make_shared<ImageEntry>();
    entry->registry = shared_from_this();
    entry->image = std::move(image);
    entry->sampler = std::move(sampler);
    _entries.push_back(entry);
    return entry;
}

uint64_t ImageRegistry::identifier(ImageEntry& entry)
{
    if (!_backend || !entry.image || !entry.sampler)
    {
        return 0;
    }
    if (ImGui::GetCurrentContext() != _context)
    {
        throw std::logic_error("Image belongs to another UI context");
    }
    const auto generation = _backend->textureGeneration();
    if (!entry.identifier || entry.generation != generation)
    {
        entry.identifier = _backend->registerTexture(*entry.image, *entry.sampler);
        entry.generation = generation;
    }
    return entry.identifier;
}

void ImageRegistry::beginFrame()
{
    if (!_backend)
    {
        return;
    }
    // Orphaned images belong only to already-submitted UI frames. Retain the
    // descriptor and its exact native owners until those submissions complete.
    for (const auto& entry : _entries)
    {
        if (entry->retirementScheduled)
        {
            continue;
        }
        if (entry.use_count() != 1)
        {
            continue;
        }
        _engine->cleanupManager().defer([weak = weak_from_this(), entry]()
        {
            if (auto registry = weak.lock(); registry && registry->_backend)
            {
                if (entry->identifier && entry->generation == registry->_backend->textureGeneration())
                {
                    registry->_backend->removeTexture(entry->identifier);
                }
                std::erase(registry->_entries, entry);
            }
        });
        entry->retirementScheduled = true;
        // Preserve the existing profiling counter while sharing registration.
        base::CpuProfileScope::count("ui.retiredTexturePreviews", 1);
    }
}

void ImageRegistry::shutdown()
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
