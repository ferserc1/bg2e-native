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

#include <bg2e/db/scene_bg2.hpp>

#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>

#include <stdexcept>

namespace bg2e::db {

std::shared_ptr<scene::Node> loadSceneBg2(const std::filesystem::path& path,
    const Bg2Mesh& model, draw::Engine& engine, scene::SceneProgressCallback onProgress)
{
    if (!model.mesh)
    {
        throw std::invalid_argument("BG2 geometry is empty");
    }
    auto drawable = std::make_shared<draw::Drawable>();
    drawable->setName(path.stem().string());
    drawable->setMesh(std::make_shared<geo::MeshPNUUT>(*model.mesh));
    for (uint32_t i = 0; i < model.materials.size(); ++i)
    {
        const auto& material = model.materials[i];
        drawable->setMaterial(draw::cloneMaterialAttributes(material), i);
        drawable->setSubmeshName(model.submeshNames.at(i), i);
        drawable->setSubmeshGroupName(material.groupName(), i);
        drawable->setSubmeshVisibility(material.visible(), i);
    }
    int textures = 0;
    for (const auto& material : model.materials)
    {
        textures += bool(material.albedoTexture()) + bool(material.metalnessTexture()) +
            bool(material.roughnessTexture()) + bool(material.normalTexture()) +
            bool(material.aoTexture()) + bool(material.lightEmissionTexture());
    }
    int processed = 0;
    drawable->load(&engine, [&]
    {
        if (onProgress)
        {
            onProgress("texture", ++processed, textures + 1);
        }
    });
    auto node = std::make_shared<scene::Node>(path.stem().string());
    auto component = std::make_shared<scene::DrawableComponent>(drawable);
    component->setResourcePath(path);
    node->addComponent(component);
    node->addComponent(new scene::TransformComponent());
    if (model.inputJoint)
    {
        auto joint = std::make_shared<scene::InputChainJointComponent>();
        joint->setJoint(*model.inputJoint);
        node->addComponent(joint);
    }
    if (model.outputJoint)
    {
        auto joint = std::make_shared<scene::OutputChainJointComponent>();
        joint->setJoint(*model.outputJoint);
        node->addComponent(joint);
    }
    if (onProgress)
    {
        onProgress(path.filename().string(), textures + 1, textures + 1);
    }
    return node;
}

std::shared_ptr<scene::Node> loadSceneBg2(const std::filesystem::path& path,
    draw::Engine& engine, scene::SceneProgressCallback onProgress)
{
    auto model = std::unique_ptr<Bg2Mesh>(loadMeshBg2(path));
    return loadSceneBg2(path, *model, engine, std::move(onProgress));
}

}
