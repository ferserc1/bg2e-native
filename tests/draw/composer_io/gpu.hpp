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

#include <bg2e/db/scene.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <stdexcept>

namespace bg2e::draw::tests {
// Call manually at a stopped Engine boundary, using an existing disposable directory.
inline void runComposerCloneProbe(Engine& engine, const std::filesystem::path& directory)
{
    auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    auto drawable = std::make_shared<Drawable>();
    drawable->setMesh(geo::createCube(1, 1, 1));
    drawable->load(&engine);
    auto original = std::make_shared<scene::Node>("Original");
    original->addComponent(new scene::DrawableComponent(drawable));
    auto copy = original->clone();
    copy->setName("Independent copy");
    auto cloned = copy->drawable()->drawDrawable();
    require(cloned != drawable && cloned->mesh() != drawable->mesh() && cloned->drawMesh() != drawable->drawMesh(),
        "Cloned CPU/GPU geometry is shared");
    require(cloned->drawMaterial(0) != drawable->drawMaterial(0), "Cloned material is shared");
    auto originalTexture = drawable->drawMaterial(0)->texture(TextureSlot::Albedo);
    auto copyTexture = cloned->drawMaterial(0)->texture(TextureSlot::Albedo);
    require(originalTexture != copyTexture && originalTexture->image() != copyTexture->image(), "Cloned GPU image is shared");
    const auto oldPosition = drawable->mesh()->vertices.at(0).position;
    cloned->mesh()->vertices.at(0).position.x += 2;
    cloned->reload();
    cloned->material().setRoughness(0.23f);
    drawable->material().setRoughness(0.81f);
    require(drawable->mesh()->vertices.at(0).position == oldPosition, "CPU clone edit changed original geometry");
    auto root = std::make_shared<scene::Node>("Clone export");
    root->addChild(original);
    root->addChild(copy);
    const auto path = directory / "composer-independent-clones.json";
    db::saveScene(root.get(), engine, path);
    auto loaded = db::loadScene(path, engine);
    auto first = loaded->rootNode()->children().at(0)->children().at(0)->drawable()->drawDrawable();
    auto second = loaded->rootNode()->children().at(0)->children().at(1)->drawable()->drawDrawable();
    require(first->mesh()->vertices.at(0).position != second->mesh()->vertices.at(0).position &&
        first->material().roughness() != second->material().roughness(), "Export merged independent cloned state");
    require(original->drawable()->resourcePath().empty() && copy->drawable()->resourcePath().empty(),
        "Complete export changed live source references");
    first->cleanup();
    second->cleanup();
    cloned->cleanup();
    drawable->cleanup();
}
}
