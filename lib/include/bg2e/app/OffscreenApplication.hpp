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

struct OffscreenConfig {
    // Render target width (default 1920)
    uint32_t width = 1920;
    // Render target height (default 1080)
    uint32_t height = 1080;
    // Output image format
    const std::string& format = "png";
    // Output image path
    const std::filesystem::path& path = "";
};

class OffscreenApplicationDelegate
{
public:
    virtual void initConfig(int argc, char ** argv, OffscreenConfig & outConfig) {}

    virtual void init(bg2e::render::Engine *) {}

    // TODO: add parameters
    virtual void render() = 0;

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

    std::shared_ptr<OffscreenApplicationDelegate> _delegate;

    std::unique_ptr<render::Engine> _engine;
};

}
