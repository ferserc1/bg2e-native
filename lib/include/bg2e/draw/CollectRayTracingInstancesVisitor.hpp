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

#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/gpu/RayTracingScene.hpp>

namespace bg2e::draw {
struct RayTracingCollectionOptions {
    bool includeTransparent = false;
    bool includeSolidTransparent = false;
    uint32_t maxInstances = 0x00ffffffu;
};
class BG2E_API CollectRayTracingInstancesVisitor {
  public:
    struct Instance {
        RenderQueue::Item item; // Retains raster buffers, BLAS, textures and identity.
        gpu::RayTracingInstance instance;
    };
    void collect(const RenderQueue& queue, RayTracingCollectionOptions options = {});
    void collect(scene::Node* root, Engine& engine, RayTracingCollectionOptions options = {});
    const std::vector<Instance>& instances() const;
    // Target scene borrows meshes; retain this collection through GPU completion.
    void populate(gpu::RayTracingScene& scene) const;
  private:
    Engine* _engine = nullptr;
    std::vector<Instance> _instances;
};
}
