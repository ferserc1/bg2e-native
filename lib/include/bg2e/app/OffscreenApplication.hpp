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
#include <bg2e/app/DrawOffscreenApplicationDelegate.hpp>

#include <string>
#include <memory>
#include <cstdint>

namespace bg2e::app {

class BG2E_API OffscreenApplication {
  public:
    OffscreenApplication();
    ~OffscreenApplication();
    OffscreenApplication(const OffscreenApplication&) = delete;
    OffscreenApplication& operator=(const OffscreenApplication&) = delete;

    void init(
        int argc, char** argv,
        const std::string& appId,
        std::shared_ptr<DrawOffscreenApplicationDelegate> delegate);

    int run();

  protected:
    void cleanup();

    std::string _appId;
    std::shared_ptr<DrawOffscreenApplicationDelegate> _delegate;
    DrawOffscreenApplicationConfig _config;
    bool _initialized = false;
    bool _delegateInitialized = false;
    bool _engineReady = false;
    bool _initializing = false;
    bool _running = false;
    bool _cleaning = false;
    std::unique_ptr<draw::Engine> _engine;
};

}
