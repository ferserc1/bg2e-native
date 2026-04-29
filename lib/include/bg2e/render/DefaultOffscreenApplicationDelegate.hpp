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

#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e::render
{

template <typename RendererT>
class BG2E_API DefaultOffscreenApplicationDelegate : public bg2e::app::OffscreenApplicationDelegate
{
public:
    virtual void initConfig(
        int argc, char *argv[],
        bg2e::app::OffscreenApplicationConfig & outConfig
    ) override;

    virtual void init(
        Engine *,
        std::shared_ptr<vulkan::Image> colorImage,
        std::shared_ptr<vulkan::Image> depthImage
    ) override;

    virtual void initFrameResources(vulkan::DescriptorSetAllocator * allocator) override;

    virtual void initScene() override;

    virtual void resize(uint32_t width, uint32_t height) override;

    virtual void frame(float delta, uint32_t frameIndex, vulkan::FrameResources&) override;

    virtual bool render(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        vulkan::FrameResources & frameResources,
        VkImageLayout colorImageLayout,
        VkImageLayout & final_color_image_la_layout
    ) override;

    virtual void didRenderFrame(
        uint32_t frameIndex,
        double elapsedMs,
        VkImageLayout colorImageLayout
    ) override;

    virtual void cleanup() override;

    RendererT * renderer();

protected:
    bg2e::render::Engine * _engine;
    std::shared_ptr<vulkan::Image> _colorImage;
    std::shared_ptr<vulkan::Image> _depthImage;

    std::unique_ptr<RendererT> _renderer;

    virtual std::shared_ptr<scene::Node> createScene() = 0;
    virtual bool continueRendering() { return false; }
};

}