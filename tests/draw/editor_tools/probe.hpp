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

// Add apps/model_edit/src to the manual probe's include path.
#include <AppDelegate.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/ui/DrawMaterialEditor.hpp>

#include <cmath>
#include <stdexcept>

namespace model_edit::tests {
// Destructive manual probe: use a disposable initialized draw_model_edit session,
// after the first renderer attachment, at a stopped submission boundary.
inline void runEditorToolsProbe(const std::shared_ptr<AppDelegate>& delegate, bg2e::draw::Engine& engine)
{
    using namespace bg2e;
    auto require = [](bool value)
    {
        if (!value)
        {
            throw std::runtime_error("Editor tool probe failed");
        }
    };
    auto drawable = std::make_shared<draw::Drawable>();
    drawable->setMesh(geo::createCube(2, 2, 2));
    drawable->load(&engine);
    auto node = std::make_shared<scene::Node>("Probe mesh");
    node->addComponent(new scene::DrawableComponent(drawable));
    auto root = std::make_shared<scene::Node>("Probe document");
    root->addChild(node);
    auto stage = delegate->stage();
    stage->prepareTools(root);
    stage->adopt(root, "editor-probe.json");
    delegate->setScene(delegate->scene());
    delegate->prepareFrame();
    delegate->selectNode(node, 0, false);
    delegate->prepareFrame();
    require(delegate->selection()->selectedMesh() == drawable.get());

    const auto albedo = drawable->material().albedo();
    ui::DrawMaterialEditor editor;
    editor.init(&engine, delegate);
    editor.setSelectionManager(delegate->selection());
    const auto generation = stage->document().generation;
    editor.setEditGuard([stage, generation]
    {
        return stage->document().generation == generation;
    });
    editor.onChanged([delegate](auto& renderer)
    {
        delegate->notifyEdited(renderer);
    });
    require(editor.queuePatch(utils::MaterialModifier(std::string("{\"roughness\":0.17}"))));
    delegate->prepareFrame();
    require(std::abs(drawable->material().roughness() - 0.17f) < 0.0001f && stage->document().dirty);
    require(drawable->materialDefinition().roughness() == drawable->material().roughness());
    require(drawable->material().albedo().r == albedo.r);

    auto image = drawable->drawMaterial(0)->normalTexture()->imageResource();
    require(editor.queueTextureSettings(draw::TextureSlot::Normal, [](auto& settings)
    {
        settings.setAddressModeU(base::Texture::AddressModeClampToEdge);
    }));
    delegate->prepareFrame();
    require(drawable->drawMaterial(0)->normalTexture()->imageResource() == image);
    require(drawable->materialDefinition().normalTexture()->addressModeU() == base::Texture::AddressModeClampToEdge);

    delegate->duplicateSelection();
    delegate->prepareFrame();
    require(root->children().size() == 2);
    auto copy = root->children().back()->drawable()->drawable();
    require(copy != drawable && copy->mesh() != drawable->mesh());
    require(copy->drawMaterial(0)->normalTexture()->image() != image.get());
    const auto sourceX = drawable->mesh()->vertices.at(0).position.x;
    delegate->editGeometry(3);
    delegate->prepareFrame();
    require(std::abs(drawable->mesh()->vertices.at(0).position.x - sourceX * 0.01f) < 0.0001f);
    require(copy->mesh()->vertices.at(0).position.x == sourceX);

    unsigned staleCalls = 0;
    delegate->queueNodeEdit(node, [&staleCalls](auto&)
    {
        ++staleCalls;
    });
    require(editor.queuePatch(utils::MaterialModifier(std::string("{\"roughness\":0.9}"))));
    auto replacement = std::make_shared<scene::Node>("Replacement");
    stage->prepareTools(replacement);
    stage->adopt(replacement, "replacement-probe.json");
    delegate->setScene(delegate->scene());
    delegate->prepareFrame();
    require(staleCalls == 0 && !editor.error().empty());
    require(!delegate->selection()->selectedItem() || delegate->selection()->selectedItem()->expired());
    editor.cleanup();
}

}
