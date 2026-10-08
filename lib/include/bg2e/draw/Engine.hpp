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
#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/gpu/Common.hpp>

#include <memory>

struct SDL_Window;

namespace bg2e {

namespace gpu {
class Backend;
class Instance;
class PhysicalDevice;
class Device;
class WindowSurface;
class CleanupManager;
}

namespace draw {

// Experimental draw engine. Owns the GPU context and global lifecycle.
// The SDL window, the Backend and its shared Instance are non-owning;
// PhysicalDevice, Device and the window Surface are exclusively owned.
class BG2E_API Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void init(SDL_Window* window, gpu::Backend& backend, const EngineConfig& config);
    void cleanup();

    gpu::BackendType backendType() const;
    gpu::Instance* instance() const;
    gpu::PhysicalDevice* physicalDevice() const;
    gpu::Device* device() const;
    gpu::WindowSurface* surface() const;
    gpu::CleanupManager& cleanupManager();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}
}
