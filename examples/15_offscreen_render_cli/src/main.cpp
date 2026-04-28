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
#include <bg2e.hpp>

class MyOffscreenApplication : public bg2e::app::OffscreenApplicationDelegate
{
public:
    void initConfig(
        [[maybe_unused]] int argc, [[maybe_unused]] char ** argv,
        bg2e::app::OffscreenApplicationConfig & outConfig
    ) override {
        outConfig.width = 1920;
        outConfig.height = 1080;
    }

    void init(bg2e::render::Engine* engine) override
    {
        _engine = engine;
    }

    void initScene() override
    {

    }

    void resize(uint32_t width, uint32_t height) override
    {
    }

    void frame(float delta, uint32_t frameIndex, bg2e::render::vulkan::FrameResources& frameResources) override
    {
    }

    bool render(VkCommandBuffer cmd, uint32_t frameIndex, bg2e::render::vulkan::FrameResources& frameResources) override
    {
        return false;
    }

    void didRenderFrame(uint32_t frameIndex, double elapsedMs) override
    {
    }

protected:
    bg2e::render::Engine * _engine;
};

int main(int argc, char** argv)
{
    bg2e_log_info << "bg2 engine offscreen render CLI example" << bg2e_log_end;

    bg2e::app::OffscreenApplication app;
    app.init(
        argc, argv,
        "my-offscreen-app",
        std::make_shared<MyOffscreenApplication>()
    );

    return app.run();
}

