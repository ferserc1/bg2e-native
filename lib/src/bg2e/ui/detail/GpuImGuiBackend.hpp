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

#include "ImGuiBackend.hpp"
#include <bg2e/gpu/GuiRenderer.hpp>
#include <bg2e/draw/Engine.hpp>
#include <utility>
#include <stdexcept>

namespace bg2e::ui::detail {

// UI owns the session; native work borrows only its engine's GPU primitives.
class GpuImGuiBackend final : public ImGuiBackend {
  public:
    explicit GpuImGuiBackend(std::unique_ptr<gpu::GuiRenderer> renderer)
        : _renderer(std::move(renderer))
    {
    }

    void initialize(draw::Engine& engine) override
    {
        if (!engine.instance() || !engine.physicalDevice() || !engine.device() || !engine.surface())
        {
            throw std::invalid_argument("UI requires an initialized windowed draw engine.");
        }
        _renderer->initialize(*engine.instance(), *engine.physicalDevice(),
                              *engine.device(), *engine.surface());
    }
    void prepareFrame(gpu::CommandBuffer* command, gpu::SurfaceFrame* frame) override
    {
        _renderer->prepareFrame(command, frame);
    }
    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        _renderer->draw(command, frame);
    }
    void shutdown() override
    {
        _renderer->shutdown();
    }

    uint64_t registerTexture(gpu::Image& image, gpu::Sampler& sampler) override
    {
        return _renderer->registerTexture(image, sampler);
    }
    void removeTexture(uint64_t identifier) override
    {
        _renderer->removeTexture(identifier);
    }
    uint64_t textureGeneration() const override
    {
        return _renderer->textureGeneration();
    }

  private:
    std::unique_ptr<gpu::GuiRenderer> _renderer;
};
}
