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

#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <string>
#include <vector>

namespace bg2e::scene {
class Node;
class EnvironmentComponent;
}

namespace bg2e::draw {
enum class RenderQueueType
{
    Opaque, Transparent, SolidTransparent
};
class BG2E_API RenderQueue {
  public:
    struct Item {
        std::weak_ptr<scene::Node> node;
        std::string nodeId;
        uint32_t objectId = 0;
        uint32_t submeshIndex = 0;
        std::shared_ptr<Drawable::DrawMesh> drawMesh;
        std::shared_ptr<MaterialBase> drawMaterial;
        std::array<std::shared_ptr<Texture>, 6> textures;
        std::array<gpu::Image*, 6> textureImages{};
        std::array<gpu::Sampler*, 6> textureSamplers{};
        std::shared_ptr<gpu::RayTracingMesh> tracingMesh;
        bool rayTracingEnabled = false;
        glm::mat4 worldMatrix{1};
        MaterialData materialData;
    };

    void beginFrame(Engine& engine);
    void enqueue(scene::Node& node, const std::shared_ptr<Drawable>& drawable, uint32_t submesh, const glm::mat4& world);
    void sort(const glm::vec3& cameraPosition);
    void validate(Engine& engine) const;
    const std::vector<Item>& items() const;
    const std::vector<Item>& queue(RenderQueueType type) const;
    Engine* engine() const;
    LightUniforms& lights();
    const LightUniforms& lights() const;
    void setEnvironment(std::shared_ptr<const scene::EnvironmentComponent> environment);
    std::shared_ptr<const scene::EnvironmentComponent> environment() const;
    void clear();

  private:
    Engine* _engine = nullptr;
    std::vector<Item> _items;
    std::vector<Item> _opaque;
    std::vector<Item> _transparent;
    std::vector<Item> _solidTransparent;
    LightUniforms _lights;
    std::shared_ptr<const scene::EnvironmentComponent> _environment;
};

}
