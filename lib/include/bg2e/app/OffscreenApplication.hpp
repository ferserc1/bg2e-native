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

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>

#include <string>
#include <memory>

namespace bg2e::app {

struct OffscreenApplicationConfig
{
    uint32_t width = 1920;
    uint32_t height = 1080;
};

class OffscreenApplicationDelegate
{
public:
    virtual void initConfig(
        [[maybe_unused]] int argc, [[maybe_unused]] char ** argv,
        [[maybe_unused]] OffscreenApplicationConfig & outConfig
    ) {}

    virtual void init(bg2e::render::Engine *) {}

    virtual void initFrameResources(render::vulkan::DescriptorSetAllocator *) {}

    virtual void initScene() {}

    virtual void resize(uint32_t width, uint32_t height) {}

    virtual void frame(
        [[maybe_unused]] float delta,
        [[maybe_unused]] uint32_t frameIndex,
        [[maybe_unused]] render::vulkan::FrameResources& frameResources
    ) {}

    // Return true to generate another frame, return false to stop rendering
    virtual bool render(VkCommandBuffer, uint32_t frameIndex, render::vulkan::FrameResources& frameResources) = 0;

    virtual void didRenderFrame([[maybe_unused]] uint32_t frameIndex, [[maybe_unused]] double elapsedMs) {}

    virtual void cleanup() {}
};

class BG2E_API OffscreenApplication {
public:

    void init(
        int argc, char ** argv,
        const std::string& appId,
        std::shared_ptr<OffscreenApplicationDelegate> delegate
    );

    int run();

protected:
    void cleanup();

    OffscreenApplicationConfig _config;

    std::shared_ptr<OffscreenApplicationDelegate> _delegate;

    std::unique_ptr<render::Engine> _engine;
};

}
