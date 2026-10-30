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

#include <bg2e/draw/Texture.hpp>

#include <array>
#include <unordered_map>

namespace bg2e::draw {

// Short-lived, single-threaded import context. Engine/device must outlive this
// context and all returned resources. Shared images are read-only: reload/load
// replaces a Texture's image instead of modifying the shared pixel storage.
class BG2E_API ImageLoadContext {
  public:
    explicit ImageLoadContext(Engine* engine);
    ImageLoadContext(const ImageLoadContext&) = delete;
    ImageLoadContext& operator=(const ImageLoadContext&) = delete;

    Engine* engine() const;

    // Seed an otherwise file-backed source with already decoded pixels. The path
    // remains the reload/save source. Register before realizing this source.
    void registerPixels(const std::filesystem::path& path, std::shared_ptr<const base::Image> pixels);
    std::shared_ptr<Texture> load(const base::Texture& settings);
    std::shared_ptr<Texture> neutralTexture(bool normal);

  private:
    static std::string sourceKey(const std::filesystem::path& path);

    Engine* _engine;
    std::unordered_map<std::string, std::shared_ptr<const base::Image>> _pixels;
    std::unordered_map<std::string, std::shared_ptr<gpu::Image>> _images;
    std::array<std::shared_ptr<gpu::Image>, 2> _neutralImages;
};

}
