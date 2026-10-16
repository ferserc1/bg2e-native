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

#include <bg2e/draw/Engine.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Sampler.hpp>

namespace bg2e::draw {
// Deep-copy editable settings, snapshotting procedural generators into pixels.
BG2E_API std::shared_ptr<base::Texture> cloneTextureSettings(const base::Texture& source);

// Engine/device are borrowed and must outlive the texture and its GPU users.
// Reload/sampler edits/clone are synchronous rare operations: stop producers
// first. Rebind image views and resource sets after a load/reload operation.
class BG2E_API Texture {
  public:
    explicit Texture(Engine* engine);
    Texture(Engine* engine, std::shared_ptr<base::Texture> settings);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void load(std::shared_ptr<base::Texture> settings);
    void load(std::shared_ptr<base::Texture> settings, std::shared_ptr<gpu::Image> image);
    void load(const base::Image& pixels, const base::Texture& settings = {});
    void reload();
    void updateSampler(const base::Texture& settings);
    std::shared_ptr<Texture> clone() const;
    static std::shared_ptr<Texture> colorTexture(Engine* engine, const base::Color& color);

    Engine* engine() const
    {
        return _engine;
    }
    base::Texture* base()
    {
        return _settings.get();
    }
    const base::Texture* base() const
    {
        return _settings.get();
    }
    gpu::Image* image() const
    {
        return _image.get();
    }
    gpu::Sampler* sampler() const
    {
        return _sampler.get();
    }
    // Retained snapshots for consumers whose submissions outlive a texture reload.
    std::shared_ptr<gpu::Image> imageResource() const
    {
        return _image;
    }
    std::shared_ptr<gpu::Sampler> samplerResource() const
    {
        return _sampler;
    }

    void setColorType(base::Color::Type type)
    {
        _settings->setColorType(type);
    }
    base::Color::Type colorType() const
    {
        return _settings->colorType();
    }

  private:
    std::shared_ptr<gpu::Sampler> makeSampler(const base::Texture& settings, uint32_t levels) const;
    Engine* _engine;
    std::shared_ptr<base::Texture> _settings;
    std::shared_ptr<gpu::Image> _image;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::unique_ptr<base::Image> _memorySource;
};
}
