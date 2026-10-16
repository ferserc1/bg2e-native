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

#include <bg2e/ui/DrawMaterialEditor.hpp>
#include <bg2e/ui/DrawTextureWidgets.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/scene/Node.hpp>
#include <imgui.h>
#include <array>
#include <stdexcept>
#include <utility>

namespace bg2e::ui {
namespace {
void clearTexture(draw::Drawable* drawable, draw::MaterialBase* material, uint32_t submesh, draw::TextureSlot slot)
{
    using Setter = void (base::MaterialAttributes::*)(std::shared_ptr<base::Texture>);
    static const std::array<Setter, 6> setters{&base::MaterialAttributes::setAlbedoTexture,
        &base::MaterialAttributes::setMetalnessTexture, &base::MaterialAttributes::setRoughnessTexture,
        &base::MaterialAttributes::setNormalTexture, &base::MaterialAttributes::setAoTexture,
        &base::MaterialAttributes::setLightEmissionTexture};
    const auto setter = setters.at(size_t(slot));
    if (drawable)
    {
        (drawable->materialDefinition(submesh).*setter)(nullptr);
    }
    (material->materialAttributes().*setter)(nullptr);
    material->updateTextures();
}
}

struct DrawMaterialEditor::State {
    draw::Engine* engine = nullptr;
    std::weak_ptr<draw::DefaultRenderLoopDelegate> delegate;
    std::weak_ptr<manipulation::SelectionManager> selection;
    std::weak_ptr<draw::MaterialBase> material;
    std::function<void(draw::Renderer&)> changed;
    std::string error;
    uint64_t binding = 0;
    bool pending = false;
};
struct DrawMaterialEditor::Impl {
    std::array<DrawTextureWidgets, 6> textures;
};

DrawMaterialEditor::DrawMaterialEditor() : _state(std::make_shared<State>()), _impl(std::make_unique<Impl>())
{
}

DrawMaterialEditor::~DrawMaterialEditor() = default;

void DrawMaterialEditor::init(draw::Engine* engine, const std::shared_ptr<draw::DefaultRenderLoopDelegate>& delegate)
{
    if (!engine || !delegate)
    {
        throw std::invalid_argument("Draw material editor requires engine and delegate");
    }
    cleanup();
    _state->engine = engine;
    _state->delegate = delegate;
}

void DrawMaterialEditor::setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>& selection)
{
    if (selection && selection->executionMode() != manipulation::ToolExecutionMode::Draw)
    {
        throw std::invalid_argument("Draw editor requires draw selection");
    }
    ++_state->binding;
    _state->pending = false;
    _state->error.clear();
    _state->selection = selection;
    _state->material.reset();
}

void DrawMaterialEditor::setEditMaterial(const std::shared_ptr<draw::MaterialBase>& material)
{
    if (material && material->engine() != _state->engine)
    {
        throw std::invalid_argument("Material belongs to another draw engine");
    }
    ++_state->binding;
    _state->pending = false;
    _state->error.clear();
    _state->selection.reset();
    _state->material = material;
}

void DrawMaterialEditor::cleanup()
{
    ++_state->binding;
    _state->pending = false;
    _state->selection.reset();
    _state->material.reset();
    _state->delegate.reset();
    _state->engine = nullptr;
    _state->error.clear();
    for (auto& widget : _impl->textures)
    {
        widget.cleanup();
    }
}

bool DrawMaterialEditor::pending() const
{
    return _state->pending;
}

const std::string& DrawMaterialEditor::error() const
{
    return _state->error;
}

void DrawMaterialEditor::onChanged(std::function<void(draw::Renderer&)> callback)
{
    _state->changed = std::move(callback);
}

bool DrawMaterialEditor::queuePatch(const utils::MaterialModifier& modifier)
{
    if (!modifier.isValid())
    {
        _state->error = "Invalid material patch";
        return false;
    }
    return queueEdit([modifier](draw::Drawable* drawable, draw::MaterialBase* material, uint32_t submesh)
    {
        if (drawable)
        {
            modifier.apply(*drawable, submesh);
        }
        else
        {
            modifier.apply(*material);
        }
    });
}

bool DrawMaterialEditor::queueClearTexture(draw::TextureSlot slot)
{
    if (size_t(slot) >= 6)
    {
        return false;
    }
    return queueEdit([slot](draw::Drawable* drawable, draw::MaterialBase* material, uint32_t submesh)
    {
        clearTexture(drawable, material, submesh, slot);
    });
}

bool DrawMaterialEditor::queueEdit(std::function<void(draw::Drawable*, draw::MaterialBase*, uint32_t)> operation)
{
    auto delegate = _state->delegate.lock();
    if (!_state->engine || !delegate || _state->pending)
    {
        return false;
    }
    std::vector<manipulation::SelectionItem> targets;
    if (auto selection = _state->selection.lock())
    {
        for (const auto& item : selection->selectedItems())
        {
            if (item && item->lockDrawMesh())
            {
                targets.push_back(*item);
            }
        }
    }
    auto direct = _state->material;
    if (targets.empty() && direct.expired())
    {
        return false;
    }
    _state->pending = true;
    _state->error.clear();
    const uint64_t binding = _state->binding;
    const std::weak_ptr<State> weak = _state;
    const auto stale = [weak, binding]()
    {
        if (auto state = weak.lock(); state && state->binding == binding)
        {
            state->pending = false;
            state->error = "Material edit target or renderer changed";
        }
    };
    delegate->enqueueRendererUpdate(delegate->rendererGeneration(),
        [weak, binding, targets = std::move(targets), direct, operation = std::move(operation)](draw::Renderer& renderer)
    {
        auto state = weak.lock();
        if (!state || state->binding != binding)
        {
            return;
        }
        state->pending = false;
        bool touched = false;
        try
        {
            auto delegate = state->delegate.lock();
            auto root = delegate ? delegate->scene() : nullptr;
            for (const auto& target : targets)
            {
                auto node = target.node.lock();
                bool belongs = false;
                for (auto* ancestor = node.get(); ancestor; ancestor = ancestor->parent())
                {
                    belongs = belongs || ancestor == root.get();
                }
                auto drawable = target.lockDrawMesh();
                if (!belongs || !drawable || !drawable->isLoaded() || drawable->engine() != state->engine)
                {
                    continue;
                }
                auto material = drawable->drawMaterial(target.submesh);
                touched = true; // Partial failures still require invalidation.
                operation(drawable.get(), material.get(), target.submesh);
            }
            if (auto material = direct.lock(); material && material->engine() == state->engine)
            {
                touched = true;
                operation(nullptr, material.get(), 0);
            }
            if (!touched)
            {
                state->error = "Material target is no longer available";
            }
        }
        catch (const std::exception& error)
        {
            state->error = error.what();
        }
        catch (...)
        {
            state->error = "Material edit failed";
        }
        if (touched)
        {
            try
            {
                if (auto* deferred = dynamic_cast<draw::RendererDeferred*>(&renderer))
                {
                    deferred->resetUpscaler();
                }
                if (state->changed)
                {
                    state->changed(renderer);
                }
            }
            catch (const std::exception& error)
            {
                state->error = error.what();
            }
            catch (...)
            {
                state->error = "Material change notification failed";
            }
        }
    }, stale);
    return true;
}

bool DrawMaterialEditor::draw(UserInterface& ui)
{
    auto material = _state->material.lock();
    size_t selectedCount = material ? 1 : 0;
    if (auto selection = _state->selection.lock())
    {
        for (const auto& item : selection->selectedItems())
        {
            if (item)
            {
                if (auto mesh = item->lockDrawMesh(); mesh && mesh->isLoaded())
                {
                    if (!material)
                    {
                        material = mesh->drawMaterial(item->submesh);
                    }
                    ++selectedCount;
                }
            }
        }
    }
    if (!material || material->engine() != _state->engine)
    {
        for (auto& widget : _impl->textures)
        {
            widget.cleanup();
        }
        if (!_state->error.empty())
        {
            ImGui::TextWrapped("%s", _state->error.c_str());
        }
        return false;
    }
    ImGui::PushID(this);
    ImGui::Text("Editing %u material target(s)", unsigned(selectedCount));
    if (selectedCount > 1)
    {
        ImGui::TextUnformatted("Showing the first material; edited fields apply to all selected submeshes.");
    }
    ImGui::BeginDisabled(_state->pending);
    json::JsonObject patch;
    auto& attributes = material->materialAttributes();
    const auto scalar = [&](const char* label, const char* key, float value)
    {
        if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f))
        {
            patch[key] = std::make_shared<json::JsonNode>(value);
        }
    };
    const auto boolean = [&](const char* label, const char* key, bool value)
    {
        if (ImGui::Checkbox(label, &value))
        {
            patch[key] = std::make_shared<json::JsonNode>(value);
        }
    };
    auto color = attributes.albedo();
    if (Value::colorPicker("Albedo", color))
    {
        patch["albedo"] = std::make_shared<json::JsonNode>(color);
    }
    scalar("Metalness", "metalness", attributes.metalness());
    scalar("Roughness", "roughness", attributes.roughness());
    scalar("Alpha cutoff", "alphaCutoff", attributes.alphaCutoff());
    scalar("Sheen intensity", "sheenIntensity", attributes.sheenIntensity());
    float emission = attributes.lightEmission();
    if (ImGui::DragFloat("Emission", &emission, 0.05f, 0, 10000))
    {
        patch["lightEmission"] = std::make_shared<json::JsonNode>(emission);
    }
    float refraction = attributes.refractionFactor();
    if (ImGui::DragFloat("Refraction", &refraction, 0.01f, 0, 4))
    {
        patch["refractionFactor"] = std::make_shared<json::JsonNode>(refraction);
    }
    boolean("Transparent", "isTransparent", attributes.isTransparent());
    boolean("Solid", "isSolid", attributes.isSolid());
    boolean("Unlit", "unlit", attributes.isUnlit());
    auto fresnel = attributes.fresnelTint();
    auto sheen = attributes.sheenColor();
    if (Value::colorPicker("Fresnel", fresnel))
    {
        patch["fresnelTint"] = std::make_shared<json::JsonNode>(fresnel);
    }
    if (Value::colorPicker("Sheen", sheen))
    {
        patch["sheenColor"] = std::make_shared<json::JsonNode>(sheen);
    }
    const std::array<const char*, 6> names{"Albedo texture", "Metalness texture", "Roughness texture", "Normal texture", "AO texture", "Emission texture"};
    const std::array<const char*, 6> textureKeys{"albedoTexture", "metalnessTexture", "roughnessTexture", "normalTexture", "ambientOcclussion", "lightEmissionTexture"};
    const std::array<const char*, 6> prefixes{"albedo", "metalness", "roughness", "normal", "ambientOcclussion", "lightEmission"};
    std::array<uint32_t, 6> uv{attributes.albedoUVSet(), attributes.metalnessUVSet(), attributes.roughnessUVSet(), attributes.normalUVSet(), attributes.aoUVSet(), attributes.lightEmissionUVSet()};
    std::array<glm::vec2, 6> scale{attributes.albedoScale(), attributes.metalnessScale(), attributes.roughnessScale(), attributes.normalScale(), attributes.aoScale(), attributes.lightEmissionScale()};
    std::array<uint32_t, 6> channel{0, attributes.metalnessChannel(), attributes.roughnessChannel(), 0, attributes.aoChannel(), attributes.lightEmissionChannel()};
    std::array<bool, 6> inverted{false, attributes.metalnessInvert(), attributes.roughnessInvert(), false, false, attributes.lightEmissionInvert()};
    int clear = -1;
    for (size_t i = 0; i < names.size(); ++i)
    {
        ImGui::PushID(int(i));
        if (ImGui::CollapsingHeader(names[i]))
        {
            const auto choice = _impl->textures[i].draw(ui, names[i], material->texture(draw::TextureSlot(i)));
            if (choice.selected)
            {
                patch[textureKeys[i]] = std::make_shared<json::JsonNode>(choice.path.string());
            }
            if (choice.cleared)
            {
                clear = int(i);
            }
            int selectedUV = int(uv[i]);
            if (ImGui::Combo("UV", &selectedUV, "Set 0\0Set 1\0"))
            {
                patch[std::string(prefixes[i]) + "UV"] = std::make_shared<json::JsonNode>(uint32_t(selectedUV));
            }
            if (ImGui::DragFloat2("Scale", &scale[i].x, 0.01f))
            {
                patch[std::string(prefixes[i]) + "Scale"] = std::make_shared<json::JsonNode>(scale[i]);
            }
            if (i == 1 || i == 2 || i == 4 || i == 5)
            {
                int selectedChannel = int(channel[i]);
                if (ImGui::Combo("Channel", &selectedChannel, "Red\0Green\0Blue\0Alpha\0"))
                {
                    patch[std::string(prefixes[i]) + "Channel"] = std::make_shared<json::JsonNode>(uint32_t(selectedChannel));
                }
            }
            if (i == 1 || i == 2 || i == 5)
            {
                if (ImGui::Checkbox("Invert", &inverted[i]))
                {
                    patch[std::string(prefixes[i]) + "Invert"] = std::make_shared<json::JsonNode>(inverted[i]);
                }
            }
        }
        ImGui::PopID();
    }
    ImGui::EndDisabled();
    bool queued = false;
    if (!patch.empty())
    {
        const utils::MaterialModifier modifier(std::make_shared<json::JsonNode>(std::move(patch)));
        queued = queueEdit([modifier, clear](draw::Drawable* drawable, draw::MaterialBase* material, uint32_t submesh)
        {
            if (drawable)
            {
                modifier.apply(*drawable, submesh);
            }
            else
            {
                modifier.apply(*material);
            }
            if (clear >= 0)
            {
                clearTexture(drawable, material, submesh, draw::TextureSlot(clear));
            }
        });
    }
    else if (clear >= 0)
    {
        queued = queueClearTexture(draw::TextureSlot(clear));
    }
    if (!_state->error.empty())
    {
        ImGui::TextWrapped("%s", _state->error.c_str());
    }
    ImGui::PopID();
    return queued;
}

}
