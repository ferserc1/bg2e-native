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

#include <bg2e/bake/StandaloneBakeSceneAssembler.hpp>
#include <bg2e/bake/StandaloneBakerContext.hpp>
#include <bg2e/db/scene.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace bg2e::bake::tests {
// Disposable existing directory and initialized headless Engine, stopped producers.
// Writes only uniquely prefixed probe assets. It does not submit a bake or export a
// final product. The caller owns deletion of the disposable directory after use.
inline void runStandaloneAssemblyProbe(draw::Engine& engine, const std::filesystem::path& directory)
{
    auto require = [](bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    };
    auto reject = [&](const auto& operation)
    {
        bool rejected = false;
        try
        {
            operation();
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        require(rejected,"Invalid standalone assembly unexpectedly succeeded");
    };
    class Helper final : public scene::DrawableComponent {
      public:
        BG2E_COMPONENT_TYPE_NAME("StandaloneAssemblyHelperProbe");

        bool isSceneGeometry() const override
        {
            return false;
        }
        std::shared_ptr<scene::Component> clone() const override
        {
            return std::make_shared<Helper>();
        }
    };
    class BadSchema final : public scene::DrawableComponent {
      public:
        BG2E_COMPONENT_TYPE_NAME("StandaloneAssemblyBadSchemaProbe");

        void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&) override
        {
            throw std::runtime_error("Registered schema probe error");
        }
        std::shared_ptr<scene::Component> clone() const override
        {
            return std::make_shared<BadSchema>();
        }
    };
    auto& registry = scene::ComponentFactoryRegistry::get();
    registry.registerComponent(Helper::staticTypeName(),[]
    {
        return new Helper();
    });
    registry.registerComponent(BadSchema::staticTypeName(),[]
    {
        return new BadSchema();
    });
    const auto context = directory / "assembly-probe-context.json";
    const auto model = directory / "assembly-probe-model.bg2";
    const auto prefab = directory / "assembly-probe-prefab.json";
    const auto single = directory / "assembly-probe-single.json";
    require(!std::filesystem::exists(context) && !std::filesystem::exists(model) &&
        !std::filesystem::exists(prefab) && !std::filesystem::exists(single),"Use a fresh disposable probe directory");
    using namespace json;
    db::writeSceneDocument(JSON(JsonObject{{"scene",JSON(JsonList{})}}),context);
    db::Bg2Mesh data;
    data.mesh.reset(geo::createPlane(1,1));
    for (auto& vertex : data.mesh->vertices)
    {
        vertex.texCoord1 = vertex.texCoord0;
    }
    data.materials.resize(data.mesh->submeshes.size());
    data.submeshNames.resize(data.mesh->submeshes.size(),"Assembly material");
    data.inputJoint = base::LinkJoint();
    data.inputJoint->setOffset({1,2,3});
    db::storeMeshBg2(model,&data);
    auto bytes = [](const auto& path)
    {
        std::ifstream stream(path,std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>());
    };
    const auto originalBytes = bytes(model);
    const auto targetNode = JSON(JsonObject{
        {"type",JSON("Node")},{"name",JSON("same?/name")},
        {"components",JSON(JsonList{std::make_shared<JsonNode>(),JSON(JsonObject{{"type",JSON("NotRegisteredAssemblyProbe")}}),
            JSON(JsonObject{{"type",JSON("Drawable")},{"path",JSON(model.filename().string())}}),
            JSON(JsonObject{{"type",JSON(Helper::staticTypeName())},{"path",JSON("missing-helper.bg2")}})})},
        {"children",JSON(JsonList{})}});
    // Repeated names are legal and cannot collide because sibling positions participate.
    db::writeSceneDocument(JSON(JsonObject{{"scene",JSON(JsonList{targetNode,targetNode})}}),prefab);
    {
        std::ofstream stream(single);
        stream.exceptions(std::ios::failbit | std::ios::badbit);
        stream << targetNode->toString();
    }
    StandaloneBakeSceneAssembler assembler(engine);
    geo::Uv2AtlasOptions options;
    options.resolution = 32;
    options.paddingPixels = 1;
    auto cpu = assembler.assembleModel(context,model,options,false);
    require(cpu.targets.size() == 1 && cpu.targets[0].node == cpu.targetRoot && cpu.targets[0].uv2.valid &&
        cpu.targets[0].generatedUv2.has_value(),"CPU model/UV2 metadata mismatch");
    require(!cpu.targetRoot->drawable()->drawable()->isLoaded(),"CPU target mode unexpectedly loaded GPU resources");
    auto* joint = cpu.targetRoot->getComponent<scene::InputChainJointComponent>();
    require(joint && joint->joint().offset() == glm::vec3(1,2,3),"Model chain joint was lost");
    require(cpu.sourcePaths.size() == 2 && bytes(model) == originalBytes,"Assembly mutated source files/dependencies");
    auto first = assembler.assemblePrefab(context,prefab,options,false);
    auto second = assembler.assemblePrefab(context,prefab,options,false);
    require(first.targets.size() == 2 && first.targetRoot->children().size() == 2 &&
        first.targets[0].outputIdentity != first.targets[1].outputIdentity,"Multi-root prefab or target identities lost");
    for (size_t i = 0; i < first.targets.size(); ++i)
    {
        require(first.targets[i].outputIdentity == second.targets[i].outputIdentity &&
            first.targets[i].node->identifier() != second.targets[i].node->identifier(),
            "Output identity depends on transient node identity");
        require(!first.targets[i].node->getComponent<Helper>() && first.targets[i].node->scene() == first.scene.get(),
            "Helper geometry or stale Scene ownership survived assembly");
    }
    auto one = assembler.assemblePrefab(context,single,options,true);
    require(one.targets.size() == 1 && one.targetRoot == one.targets[0].node &&
        one.targetRoot->drawable()->drawable()->isLoaded(),"Single Node adapter/realization failed");
    // Explicitly realize a CPU candidate using the shared component hook before context preparation.
    cpu.targetRoot->loadResources(engine);
    require(cpu.targetRoot->drawable()->drawable()->isLoaded(),"Deferred target realization failed");
    const auto sourceMesh = cpu.targetRoot->drawable()->drawable()->mesh();
    auto copy = cpu.targetRoot->clone();
    require(copy->drawable()->drawable()->mesh() != sourceMesh &&
        copy->drawable()->drawable()->drawMesh() != cpu.targetRoot->drawable()->drawable()->drawMesh(),
        "Assembly clone shared mutable CPU/GPU geometry");
    copy.reset();
    auto invalid = JSON(JsonObject{{"scene",JSON(JsonList{JSON(JsonObject{
        {"name",JSON("Bad")},{"components",JSON(JsonList{JSON(JsonObject{{"type",JSON(BadSchema::staticTypeName())}})})}})})}});
    db::writeSceneDocument(invalid,prefab);
    reject([&]
    {
        assembler.assemblePrefab(context,prefab,options,false);
    });
    require(one.targets[0].node->drawable()->drawable()->isLoaded() && bytes(model) == originalBytes,
        "Failed candidate changed an earlier assembly/source");
    reject([&]
    {
        assembler.assembleModel(context,directory / "missing.bg2",options);
    });
    // A real overlap remains a valid target; no generation is requested to repair it.
    const auto originalIndices = data.mesh->indices;
    data.mesh->indices.insert(data.mesh->indices.end(),originalIndices.begin(),originalIndices.end());
    data.mesh->submeshes[0].indexCount = uint32_t(data.mesh->indices.size());
    db::storeMeshBg2(model,&data);
    auto overlap = assembler.assembleModel(context,model,std::nullopt,false);
    require(overlap.targets[0].uv2.valid && !overlap.targets[0].uv2.warning.empty(),
        "Overlap warning incorrectly rejected a target");
}
}
