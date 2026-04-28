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

#include <bg2e/app/OffscreenApplication.hpp>

namespace bg2e::app {

void OffscreenApplication::init(
    int argc, char ** argv,
    const std::string& appId,
    std::shared_ptr<OffscreenApplicationDelegate> delegate
) {
    OffscreenConfig config = {};
    _delegate = delegate;

    _delegate->initConfig(argc, argv, config);

    _engine = std::make_unique<render::Engine>();
    _engine->init(config.width, config.height);

    _delegate->init(_engine.get());

    // TODO: Create output image
}

int OffscreenApplication::run()
{
    return 0;
}

void OffscreenApplication::cleanup()
{
    _engine->cleanup();
}

}
