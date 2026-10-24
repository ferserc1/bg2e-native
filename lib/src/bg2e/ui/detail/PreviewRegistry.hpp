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

#include <bg2e/ui/TexturePreview.hpp>
#include <bg2e/draw/Texture.hpp>
#include "ImGuiBackend.hpp"
#include <vector>

namespace bg2e::ui::detail {
class PreviewRegistry;
struct PreviewEntry {
    std::weak_ptr<PreviewRegistry> registry;
    std::shared_ptr<gpu::Image> image;
    std::shared_ptr<gpu::Sampler> sampler;
    uint64_t identifier = 0;
    uint64_t generation = 0;
    bool retirementScheduled = false;
};

class PreviewRegistry : public std::enable_shared_from_this<PreviewRegistry> {
  public:
    PreviewRegistry(draw::Engine& engine, ImGuiBackend& backend);

    std::shared_ptr<TexturePreview> create(const std::shared_ptr<draw::Texture>& texture);
    void beginFrame();
    void shutdown();
    uint64_t identifier(PreviewEntry& entry);

  private:
    void* _context;
    draw::Engine* _engine;
    ImGuiBackend* _backend;
    std::vector<std::shared_ptr<PreviewEntry>> _entries;
};

}
