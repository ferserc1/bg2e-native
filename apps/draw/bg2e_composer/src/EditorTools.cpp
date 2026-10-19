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

#include "EditorUi.hpp"

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Vector.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/ui/SelectableList.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>

#include <random>

namespace bg2e_composer {

void EditorUi::bindDocument(const std::shared_ptr<AppDelegate>& delegate)
{
    auto document = delegate->document();
    if (!document || _boundGeneration == document->generation || !delegate->selection())
    {
        return;
    }
    _boundGeneration = document->generation;
    const auto generation = _boundGeneration;
    const auto weak = _delegate;
    _material.setSelectionManager(delegate->selection());
    _material.setEditGuard([weak, generation]
    {
        auto owner = weak.lock();
        return owner && owner->document() && owner->document()->generation == generation;
    });
    _material.onChanged([weak](auto& renderer)
    {
        if (auto owner = weak.lock())
        {
            owner->notifyEdited(renderer);
        }
    });
}

void EditorUi::drawTools(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    const auto document = delegate->document();
    if (!document || !document->root)
    {
        ui::Text::text("Open a model to edit it.");
        return;
    }
    auto selection = delegate->selection();
    if (ui::Button::checkBox("Add to selection", &_additive))
    {
        delegate->setAdditiveSelection(_additive);
    }
    if (ui::Value::comboBox("Highlight", {"Hidden", "Faint", "Normal", "Strong"}, _highlight))
    {
        const float levels[]{0, 0.25f, 0.8f, 1};
        delegate->setHighlight(levels[_highlight]);
    }
    std::function<void(const std::shared_ptr<scene::Node>&)> list;
    list = [&](const auto& node)
    {
        if (auto component = node->template getComponent<scene::DrawableComponent>())
        {
            auto mesh = component->drawDrawable();
            if (mesh)
            {
                for (uint32_t sub = 0; sub < mesh->submeshesCount(); ++sub)
                {
                    bool selected = selection->isSelected(node.get(), component, sub);
                    if (ui::SelectableList::item(node->name() + " / " + mesh->submeshName(sub) +
                        "##" + node->identifier() + "/" + std::to_string(sub), selected))
                    {
                        delegate->selectNode(node, sub, _additive);
                    }
                }
            }
        }
        else
        {
            bool selected = selection->isSelected(node.get());
            if (ui::SelectableList::item(node->name() + "##" + node->identifier(), selected))
            {
                delegate->selectNode(node, 0, _additive);
            }
        }
        for (const auto& child : node->children())
        {
            list(child);
        }
    };
    ui::SelectableList::beginList();
    list(document->root);
    ui::SelectableList::endList();
    auto target = selection->selectedNode();
    if (target)
    {
        auto name = target->name();
        if (ui::Value::text("Node name", name))
        {
            delegate->queueNodeEdit(target->shared_from_this(), [name](auto& node)
            {
                node.setName(name);
            });
        }
        if (auto transform = target->transform())
        {
            auto matrix = transform->matrix();
            if (ui::Vector::mat4("Node transform", matrix))
            {
                delegate->queueNodeEdit(target->shared_from_this(), [matrix](auto& node)
                {
                    node.transform()->setMatrix(matrix);
                });
            }
        }
        if (ui::Button::button("Duplicate node"))
        {
            delegate->duplicateSelection();
        }
        if (ui::Button::button("Remove child node"))
        {
            delegate->removeSelection();
        }
    }
    if (auto item = selection->selectedItem(); item && item->lockDrawMesh())
    {
        auto mesh = item->lockDrawMesh();
        auto name = mesh->submeshName(item->submesh);
        auto group = mesh->submeshGroupName(item->submesh);
        auto visible = mesh->submeshVisibility(item->submesh);
        auto editMetadata = [&](const auto& operation, bool all = false)
        {
            const auto targets = all ? selection->selectedItems() : std::vector<std::shared_ptr<manipulation::SelectionItem>>{item};
            for (const auto& current : targets)
            {
                const auto snapshot = *current;
                delegate->queueNodeEdit(current->node, [snapshot, operation](auto&)
                {
                    if (auto mesh = snapshot.lockDrawMesh())
                    {
                        operation(*mesh, snapshot.submesh);
                    }
                });
            }
        };
        if (ui::Value::text("Submesh name", name))
        {
            editMetadata([name](auto& mesh, auto sub)
            {
                mesh.setSubmeshName(name, sub);
            });
        }
        if (ui::Value::text("Group", group))
        {
            editMetadata([group](auto& mesh, auto sub)
            {
                mesh.setSubmeshGroupName(group, sub);
            }, true);
        }
        if (ui::Button::checkBox("Visible", &visible))
        {
            editMetadata([visible](auto& mesh, auto sub)
            {
                mesh.setSubmeshVisibility(visible, sub);
            }, true);
        }
        ui::Text::separator("Geometry (whole selected meshes)");
        const char* operations[]{"Z up to Y up", "Center geometry", "Put on floor", "cm to m"};
        for (unsigned i = 0; i < 4; ++i)
        {
            if (ui::Button::button(operations[i]))
            {
                delegate->editGeometry(i);
            }
        }
        if (ui::Button::button("Copy material"))
        {
            delegate->copyMaterial();
        }
        if (ui::Button::button("Paste material to selection"))
        {
            delegate->pasteMaterial();
        }
        if (ui::Button::button("Apply material JSON patch..."))
        {
            delegate->applyMaterialPatch(app::FileDialog::getOpenFilePath({{"Partial material", "json"}}));
        }
        ui::Text::separator("Material and textures");
        if (_ui)
        {
            _material.draw(*_ui);
        }
    }
}

void EditorUi::drawEnvironment(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    _floorVisible = delegate->stage()->floorVisible();
    _floorHeight = delegate->stage()->floorHeight();
    if (ui::Button::checkBox("Show stage floor", &_floorVisible) |
        ui::Numeric::drag("Floor height", &_floorHeight, 0.05f))
    {
        delegate->setFloor(_floorVisible, _floorHeight);
    }
    auto fill = delegate->stage()->fillLightEnabled();
    if (delegate->stage()->fillLightNode() && delegate->stage()->fillLightNode()->light() &&
        ui::Button::checkBox("Stage fill light", &fill))
    {
        delegate->setFillLight(fill);
    }
    auto document = delegate->document();
    if (!document || !document->root)
    {
        ui::Text::text("Open a document to edit its environment and lights.");
        return;
    }
    auto environment = delegate->stage()->environmentNode();
    auto editEnvironment = [&](std::function<void(scene::EnvironmentComponent&)> operation)
    {
        delegate->editEnvironment(std::move(operation));
    };
    auto editedEnvironment = environment
        ? std::dynamic_pointer_cast<scene::EnvironmentComponent>(environment->environment()->clone())
        : std::make_shared<scene::EnvironmentComponent>();
    if (const auto* info = reflection::TypeRegistry::get().type(editedEnvironment->typeName()))
    {
        if (ui::ReflectionWidget::drawProperties(editedEnvironment.get(), *info))
        {
            editEnvironment([editedEnvironment](auto& env)
            {
                env.setEnvironmentImage(editedEnvironment->environmentImage());
                env.setSkyboxVisible(editedEnvironment->skyboxVisible());
                env.setSkyboxBlur(editedEnvironment->skyboxBlur());
                env.setSkyHorizonColor(editedEnvironment->skyHorizonColor());
                env.setSkyZenithColor(editedEnvironment->skyZenithColor());
                env.setGroundColor(editedEnvironment->groundColor());
            });
        }
    }
    const char* presets[]{"mirrored_hall_4k.hdr", "theater_01_4k.hdr", "autumn_field_4k.hdr",
        "gothic_manor_01_4k.hdr", "HDR_multi_nebulae_1.hdr", "black.jpg"};
    uint32_t preset = environment && !environment->environment()->environmentImage().empty() ? 7 : 0;
    if (environment)
    {
        const auto file = std::filesystem::path(environment->environment()->environmentImage()).filename().string();
        for (uint32_t i = 0; i < 6; ++i)
        {
            if (file == presets[i])
            {
                preset = i + 1;
            }
        }
    }
    if (ui::Value::comboBox("Environment preset", {"Procedural sky", presets[0], presets[1], presets[2], presets[3], presets[4], presets[5], "Current / custom image"}, preset))
    {
        if (preset == 0)
        {
            editEnvironment([](auto& env)
            {
                env.setEnvironmentImage("");
            });
        }
        else if (preset <= 6)
        {
            const auto path = (base::PlatformTools::assetPath() / presets[preset - 1]).string();
            editEnvironment([path](auto& env)
            {
                env.setEnvironmentImage(path);
            });
        }
    }
    if (ui::Button::button("Custom HDR..."))
    {
        const auto path = app::FileDialog::getOpenFilePath({{"Environment image", "hdr,png,jpg"}});
        if (!path.empty())
        {
            editEnvironment([path](auto& env)
            {
                env.setEnvironmentImage(path.string());
            });
        }
    }
    if (ui::Button::button("Save environment preset..."))
    {
        delegate->environmentPreset(app::FileDialog::getSaveFilePath({{"Environment scene", "json"}}), true);
    }
    if (ui::Button::button("Restore environment preset..."))
    {
        delegate->environmentPreset(app::FileDialog::getOpenFilePath({{"Environment scene", "json"}}), false);
    }
    const auto defaults = base::PlatformTools::settingsPath() / "draw_bg2e_composer_environment.json";
    if (ui::Button::button("Save as my environment default"))
    {
        delegate->environmentPreset(defaults, true);
    }
    if (ui::Button::button("Restore my environment default"))
    {
        delegate->environmentPreset(defaults, false);
    }
    if (auto projection = delegate->stage()->camera()->template projection<math::PerspectiveProjection>())
    {
        auto fov = projection->fov();
        auto nearPlane = projection->near();
        auto farPlane = projection->far();
        const bool changed = ui::Numeric::slider("Camera field of view", &fov, 10, 150) |
            ui::Numeric::drag("Near plane", &nearPlane, 0.001f, 0.001f, 100) |
            ui::Numeric::drag("Far plane", &farPlane, 1, 1, 100000);
        if (changed)
        {
            delegate->setCameraIntrinsics(fov, nearPlane, farPlane);
        }
    }
    auto drawLight = [&](const std::shared_ptr<scene::Node>& node)
    {
        ui::Group::pushId(int(std::hash<std::string>{}(node->identifier()) & 0x7fffffff));
        ui::Text::text(node->name());
        auto light = std::dynamic_pointer_cast<scene::LightComponent>(node->light()->clone());
        auto transform = node->transform()
            ? std::dynamic_pointer_cast<scene::TransformComponent>(node->transform()->clone())
            : std::make_shared<scene::TransformComponent>();
        bool changed = false;
        if (const auto* info = reflection::TypeRegistry::get().type(transform->typeName()))
        {
            changed |= ui::ReflectionWidget::drawProperties(transform.get(), *info);
        }
        if (const auto* info = reflection::TypeRegistry::get().type(light->typeName()))
        {
            changed |= ui::ReflectionWidget::drawProperties(light.get(), *info);
        }
        if (changed)
        {
            delegate->queueNodeEdit(node, [light, transform](auto& target)
            {
                if (!target.transform())
                {
                    target.addComponent(new scene::TransformComponent());
                }
                target.transform()->setMatrix(transform->matrix());
                target.light()->light() = light->light();
            }, true);
        }
        if (ui::Button::button("Remove light"))
        {
            delegate->queueNodeEdit(node, [](auto& target)
            {
                if (auto component = target.light())
                {
                    target.removeComponent(component->shared_from_this());
                }
            }, true);
        }
        ui::Group::popId();
    };
    ui::Text::separator("Stage light");
    if (auto fillNode = delegate->stage()->fillLightNode(); fillNode && fillNode->light())
    {
        drawLight(fillNode);
    }
    ui::Text::separator("Document lights (maximum five)");
    unsigned count = 0;
    std::function<void(const std::shared_ptr<scene::Node>&)> lights;
    lights = [&](const auto& node)
    {
        if (node->light())
        {
            ++count;
            drawLight(node);
        }
        for (const auto& child : node->children())
        {
            lights(child);
        }
    };
    lights(document->root);
    if (ui::Button::button("Add light", false, count >= 5))
    {
        delegate->queueNodeEdit(document->root, [](auto& root)
        {
            unsigned count = 0;
            std::function<void(scene::Node&)> countLights;
            countLights = [&](auto& node)
            {
                count += bool(node.light());
                for (const auto& child : node.children())
                {
                    countLights(*child);
                }
            };
            countLights(root);
            if (count >= 5)
            {
                throw std::invalid_argument("The editor supports at most five document lights");
            }
            auto light = std::make_shared<scene::Node>("Light");
            light->addComponent(new scene::LightComponent());
            static std::mt19937 generator{std::random_device{}()};
            std::uniform_real_distribution<float> horizontal{-3.0f, 3.0f};
            std::uniform_real_distribution<float> vertical{0.0f, 3.0f};
            const auto x = horizontal(generator);
            const auto y = vertical(generator);
            const auto z = horizontal(generator);
            light->addComponent(scene::TransformComponent::makeTranslated(x, y, z));
            root.addChild(light);
        }, true);
    }
}

}
