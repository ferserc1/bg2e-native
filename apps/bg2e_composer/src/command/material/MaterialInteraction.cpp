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

#include "MaterialInteraction.hpp"

#include <set>
#include <stdexcept>
#include <typeinfo>
#include <utility>

namespace bg2e_composer::command::material {
namespace {
using namespace bg2e;
using namespace bg2e::app;
using Attributes = base::MaterialAttributes;

struct Values {
    bool isTransparent;
    bool isSolid;
    bool isUnlit;
    float refractionFactor;
    float alphaCutoff;
    base::Color albedo;
    glm::vec2 albedoScale;
    uint32_t albedoUVSet;
    float metalness;
    glm::vec2 metalnessScale;
    uint32_t metalnessChannel;
    bool metalnessInvert;
    uint32_t metalnessUVSet;
    float roughness;
    glm::vec2 roughnessScale;
    uint32_t roughnessChannel;
    bool roughnessInvert;
    uint32_t roughnessUVSet;
    glm::vec2 normalScale;
    uint32_t normalUVSet;
    base::Color fresnelTint;
    float sheenIntensity;
    base::Color sheenColor;
    glm::vec2 aoScale;
    uint32_t aoChannel;
    uint32_t aoUVSet;
    float lightEmission;
    glm::vec2 lightEmissionScale;
    uint32_t lightEmissionChannel;
    bool lightEmissionInvert;
    uint32_t lightEmissionUVSet;

    static Values capture(const Attributes& attributes)
    {
        return {
            attributes.isTransparent(), attributes.isSolid(), attributes.isUnlit(),
            attributes.refractionFactor(), attributes.alphaCutoff(), attributes.albedo(),
            attributes.albedoScale(), attributes.albedoUVSet(), attributes.metalness(),
            attributes.metalnessScale(), attributes.metalnessChannel(), attributes.metalnessInvert(),
            attributes.metalnessUVSet(), attributes.roughness(), attributes.roughnessScale(),
            attributes.roughnessChannel(), attributes.roughnessInvert(), attributes.roughnessUVSet(),
            attributes.normalScale(), attributes.normalUVSet(), attributes.fresnelTint(),
            attributes.sheenIntensity(), attributes.sheenColor(), attributes.aoScale(),
            attributes.aoChannel(), attributes.aoUVSet(), attributes.lightEmission(),
            attributes.lightEmissionScale(), attributes.lightEmissionChannel(), attributes.lightEmissionInvert(),
            attributes.lightEmissionUVSet()};
    }

    void write(Attributes& attributes) const
    {
        attributes.setIsTransparent(isTransparent);
        attributes.setIsSolid(isSolid);
        attributes.setIsUnlit(isUnlit);
        attributes.setRefractionFactor(refractionFactor);
        attributes.setAlphaCutoff(alphaCutoff);
        attributes.setAlbedo(albedo);
        attributes.setAlbedoScale(albedoScale);
        attributes.setAlbedoUVSet(albedoUVSet);
        attributes.setMetalness(metalness);
        attributes.setMetalnessScale(metalnessScale);
        attributes.setMetalnessChannel(metalnessChannel);
        attributes.setMetalnessInvert(metalnessInvert);
        attributes.setMetalnessUVSet(metalnessUVSet);
        attributes.setRoughness(roughness);
        attributes.setRoughnessScale(roughnessScale);
        attributes.setRoughnessChannel(roughnessChannel);
        attributes.setRoughnessInvert(roughnessInvert);
        attributes.setRoughnessUVSet(roughnessUVSet);
        attributes.setNormalScale(normalScale);
        attributes.setNormalUVSet(normalUVSet);
        attributes.setFresnelTint(fresnelTint);
        attributes.setSheenIntensity(sheenIntensity);
        attributes.setSheenColor(sheenColor);
        attributes.setAoScale(aoScale);
        attributes.setAoChannel(aoChannel);
        attributes.setAoUVSet(aoUVSet);
        attributes.setLightEmission(lightEmission);
        attributes.setLightEmissionScale(lightEmissionScale);
        attributes.setLightEmissionChannel(lightEmissionChannel);
        attributes.setLightEmissionInvert(lightEmissionInvert);
        attributes.setLightEmissionUVSet(lightEmissionUVSet);
    }

    bool operator==(const Values& other) const
    {
        const auto colorEqual = [](const base::Color& a, const base::Color& b)
        {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        };
        return isTransparent == other.isTransparent &&
            isSolid == other.isSolid &&
            isUnlit == other.isUnlit &&
            refractionFactor == other.refractionFactor &&
            alphaCutoff == other.alphaCutoff &&
            colorEqual(albedo, other.albedo) &&
            albedoScale == other.albedoScale &&
            albedoUVSet == other.albedoUVSet &&
            metalness == other.metalness &&
            metalnessScale == other.metalnessScale &&
            metalnessChannel == other.metalnessChannel &&
            metalnessInvert == other.metalnessInvert &&
            metalnessUVSet == other.metalnessUVSet &&
            roughness == other.roughness &&
            roughnessScale == other.roughnessScale &&
            roughnessChannel == other.roughnessChannel &&
            roughnessInvert == other.roughnessInvert &&
            roughnessUVSet == other.roughnessUVSet &&
            normalScale == other.normalScale &&
            normalUVSet == other.normalUVSet &&
            colorEqual(fresnelTint, other.fresnelTint) &&
            sheenIntensity == other.sheenIntensity &&
            colorEqual(sheenColor, other.sheenColor) &&
            aoScale == other.aoScale &&
            aoChannel == other.aoChannel &&
            aoUVSet == other.aoUVSet &&
            lightEmission == other.lightEmission &&
            lightEmissionScale == other.lightEmissionScale &&
            lightEmissionChannel == other.lightEmissionChannel &&
            lightEmissionInvert == other.lightEmissionInvert &&
            lightEmissionUVSet == other.lightEmissionUVSet;
    }
};

struct State {
    Values live;
    Values definition;

    bool operator==(const State&) const = default;
};

bool sameResources(const Attributes& a, const Attributes& b)
{
    return a.albedoTexture() == b.albedoTexture() && a.normalTexture() == b.normalTexture() &&
        a.metalnessTexture() == b.metalnessTexture() && a.roughnessTexture() == b.roughnessTexture() &&
        a.aoTexture() == b.aoTexture() && a.lightEmissionTexture() == b.lightEmissionTexture() &&
        a.indirectTextureMode() == b.indirectTextureMode() && a.isUpdated() == b.isUpdated();
}

class MaterialInteraction final : public CommandTransaction {
  public:
    MaterialInteraction(std::weak_ptr<StageScene> stage, ui::MaterialEditTargets targets,
        std::function<void()> effects)
        : _stage(std::move(stage)), _selection(std::move(targets)), _effects(std::move(effects))
    {
    }

    const CommandInfo& info() const noexcept override
    {
        return _info;
    }

    CommandMemory memory() const override
    {
        // Baseline/final values, temporary proposals and immutable CPU payload.
        return {sizeof(*this) + 65536 + _selection.selection.size() *
            (sizeof(Target) + 2 * sizeof(State) + sizeof(manipulation::SelectionItem)), {}};
    }

    CommandMemory memoryForUpdate(const std::any& value) const override
    {
        const auto& intent = std::any_cast<const ui::MaterialEditIntent&>(value);
        if (!intent.cpuValuesOnly || !intent.patch || intent.sampling || intent.indirectMode ||
            intent.ordinaryAo || intent.linearSlot >= 0 || intent.clearSlot >= 0)
        {
            throw std::invalid_argument("Resource edits require a discrete material command");
        }
        return memory();
    }

    CommandResult begin() noexcept override
    {
        try
        {
            auto stage = _stage.lock();
            if (!stage || _selection.selection.empty() || !_selection.direct.expired())
            {
                return {CommandStatus::Rejected, "Material interaction requires document targets"};
            }
            std::set<draw::MaterialBase*> seen;
            std::set<std::pair<draw::Drawable*, uint32_t>> receivers;
            for (const auto& item : _selection.selection)
            {
                auto node = item.node.lock();
                auto mesh = item.lockMesh();
                if (mesh && !receivers.emplace(mesh.get(), item.submesh).second)
                {
                    continue;
                }
                auto material = mesh && mesh->isLoaded() ? mesh->drawMaterial(item.submesh) : nullptr;
                if (!node || !stage->contains(node.get()) || !material ||
                    typeid(*material) != typeid(draw::MaterialBase) || !seen.insert(material.get()).second)
                {
                    return {CommandStatus::Rejected, "Material target is stale, shared or custom"};
                }
                State state{Values::capture(material->materialAttributes()),
                    Values::capture(mesh->materialDefinition(item.submesh))};
                _targets.push_back({item, material, state, state});
            }
            return {};
        }
        catch (...)
        {
            return {CommandStatus::Rejected, "Cannot capture material interaction"};
        }
    }

    CommandResult update(const std::any& value) noexcept override
    {
        try
        {
            if (!valid())
            {
                return {CommandStatus::Rejected, "Material interaction target changed"};
            }
            const auto& intent = std::any_cast<const ui::MaterialEditIntent&>(value);
            std::vector<State> next;
            next.reserve(_targets.size());
            bool effective = false;
            bool changed = false;
            for (const auto& target : _targets)
            {
                auto material = target.material.lock();
                auto mesh = target.item.lockMesh();
                const auto& live = material->materialAttributes();
                const auto& definition = mesh->materialDefinition(target.item.submesh);
                // Propose against CPU values only; never copy metadata strings,
                // texture settings or retained sources for a slider update.
                const Attributes empty;
                Attributes proposedLive, proposedDefinition;
                Values::capture(live).write(proposedLive);
                Values::capture(definition).write(proposedDefinition);
                intent.patch->apply(proposedLive);
                intent.patch->apply(proposedDefinition);
                if (!sameResources(empty, proposedLive) || !sameResources(empty, proposedDefinition))
                {
                    return {CommandStatus::Rejected, "A value preview cannot replace texture resources"};
                }
                next.push_back({Values::capture(proposedLive), Values::capture(proposedDefinition)});
                effective = effective || !(next.back() == target.after);
                changed = changed || !(next.back() == target.before);
            }
            if (!effective)
            {
                return {};
            }
            for (size_t i = 0; i < _targets.size(); ++i)
            {
                write(_targets[i], next[i]);
            }
            try
            {
                _effects();
            }
            catch (...)
            {
                for (const auto& target : _targets)
                {
                    write(target, target.after);
                }
                try
                {
                    _effects();
                }
                catch (...)
                {
                    return {CommandStatus::Faulted, "Material preview recovery failed"};
                }
                return {CommandStatus::Failed, "Material preview restored"};
            }
            for (size_t i = 0; i < _targets.size(); ++i)
            {
                _targets[i].after = next[i];
            }
            _changed = changed;
            return {CommandStatus::Applied, {}};
        }
        catch (...)
        {
            return {CommandStatus::Rejected, "Material preview preparation failed"};
        }
    }

    bool changed() const noexcept override
    {
        return _changed;
    }

    CommandResult execute() noexcept override
    {
        return {CommandStatus::Rejected, "Material interactions must be adopted"};
    }

    CommandResult undo() noexcept override
    {
        return restore(false);
    }

    CommandResult redo() noexcept override
    {
        return restore(true);
    }

  private:
    struct Target {
        manipulation::SelectionItem item;
        std::weak_ptr<draw::MaterialBase> material;
        State before, after;
    };
    std::weak_ptr<StageScene> _stage;
    ui::MaterialEditTargets _selection;
    std::function<void()> _effects;
    std::vector<Target> _targets;
    bool _changed = false;
    CommandInfo _info{"Edit material values", "material.values"};

    bool valid() const
    {
        auto stage = _stage.lock();
        for (const auto& target : _targets)
        {
            auto node = target.item.node.lock();
            auto mesh = target.item.lockMesh();
            auto material = target.material.lock();
            if (!stage || !node || !stage->contains(node.get()) || !mesh || !mesh->isLoaded() ||
                !material || mesh->drawMaterial(target.item.submesh) != material)
            {
                return false;
            }
        }
        return !_targets.empty();
    }

    static void write(const Target& target, const State& state)
    {
        state.live.write(target.material.lock()->materialAttributes());
        state.definition.write(target.item.lockMesh()->materialDefinition(target.item.submesh));
    }

    CommandResult restore(bool forward) noexcept
    {
        try
        {
            if (!valid())
            {
                return {CommandStatus::Rejected, "Material interaction target changed"};
            }
            std::vector<State> recovery;
            recovery.reserve(_targets.size());
            for (const auto& target : _targets)
            {
                recovery.push_back({Values::capture(target.material.lock()->materialAttributes()),
                    Values::capture(target.item.lockMesh()->materialDefinition(target.item.submesh))});
            }
            for (const auto& target : _targets)
            {
                write(target, forward ? target.after : target.before);
            }
            try
            {
                _effects();
            }
            catch (...)
            {
                for (size_t i = 0; i < _targets.size(); ++i)
                {
                    write(_targets[i], recovery[i]);
                }
                try
                {
                    _effects();
                }
                catch (...)
                {
                    return {CommandStatus::Faulted, "Material restoration recovery failed"};
                }
                return {CommandStatus::Failed, "Material restoration failed; previous values retained"};
            }
            return {CommandStatus::Applied, {}};
        }
        catch (...)
        {
            return {CommandStatus::Rejected, "Material restoration preparation failed"};
        }
    }
};
}

std::unique_ptr<bg2e::app::CommandTransaction> materialInteraction(
    std::weak_ptr<StageScene> stage, bg2e::ui::MaterialEditTargets targets,
    std::function<void()> effects)
{
    return std::make_unique<MaterialInteraction>(std::move(stage), std::move(targets), std::move(effects));
}

}
