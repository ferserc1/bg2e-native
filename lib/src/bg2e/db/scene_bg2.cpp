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
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>

namespace bg2e::db {

std::shared_ptr<scene::Node> loadSceneBg2(
    const std::filesystem::path& filePath,
    render::Engine* engine)
{
    auto model = std::unique_ptr<Bg2Mesh>(loadMeshBg2(filePath));

    auto drawable = std::make_shared<scene::Drawable>();
    drawable->setName(filePath.stem().string());
    drawable->setMesh(model->mesh);
    for (uint32_t index = 0; index < model->materials.size(); ++index)
    {
        auto& material = model->materials[index];
        drawable->setMaterial(material, index);
        drawable->setSubmeshName(material.name(), index);
        drawable->setSubmeshGroupName(material.groupName(), index);
        drawable->setSubmeshVisibility(material.visible(), index);
    }
    drawable->load(engine);

    auto result = std::make_shared<scene::Node>(filePath.stem().string());
    result->addComponent(std::make_shared<scene::DrawableComponent>(drawable));
    result->addComponent(std::make_shared<scene::TransformComponent>());

    if (model->inputJoint)
    {
        auto component = std::make_shared<scene::InputChainJointComponent>();
        component->setJoint(*model->inputJoint);
        result->addComponent(component);
    }

    if (model->outputJoint)
    {
        auto component = std::make_shared<scene::OutputChainJointComponent>();
        component->setJoint(*model->outputJoint);
        result->addComponent(component);
    }

    return result;
}

std::shared_ptr<scene::Node> loadSceneBg2(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    render::Engine* engine)
{
    return loadSceneBg2(basePath / fileName, engine);
}

}
