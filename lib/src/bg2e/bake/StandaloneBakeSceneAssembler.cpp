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

#include <bg2e/bake/StandaloneBakeSceneAssembler.hpp>
#include <bg2e/bake/Uv2Reload.hpp>
#include <bg2e/bake/UvSurfaceMesh.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/db/scene.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace bg2e::bake {
namespace {
std::filesystem::path inputPath(const std::filesystem::path& path)
{
    if (path.empty() || !std::filesystem::is_regular_file(path))
    {
        throw std::invalid_argument("Standalone assembly requires an existing input file: " + path.string());
    }
    return std::filesystem::absolute(path).lexically_normal();
}

std::string extension(const std::filesystem::path& path)
{
    auto value = path.extension().string();
    for (auto& character : value)
    {
        if (character >= 'A' && character <= 'Z')
        {
            character = char(character-'A'+'a');
        }
    }
    return value;
}

std::string segment(const std::string& name, size_t index)
{
    std::string result;
    for (const unsigned char character : name)
    {
        const bool safe = (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') ||
            character == '_' || character == '-';
        result.push_back(safe ? char(character) : '_');
    }
    return (result.empty() ? "node" : result) + "-" + std::to_string(index);
}

std::shared_ptr<scene::Scene> readScene(const std::filesystem::path& path, bool allowNode)
{
    std::shared_ptr<json::JsonNode> document;
    if (allowNode)
    {
        // readSceneDocument intentionally requires a scene array. The legacy
        // single-Node prefab shape needs only this representation adapter; the
        // engine JSON parser and shared graph factories remain authoritative.
        std::ifstream stream(path,std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("Could not open standalone prefab: " + path.string());
        }
        document = json::JsonParser(&stream).parse();
        if (stream.bad())
        {
            throw std::runtime_error("Could not read standalone prefab: " + path.string());
        }
    }
    else
    {
        document = db::readSceneDocument(path);
    }
    json::ObjectReader reader(document);
    if (!reader.isValid())
    {
        throw std::invalid_argument("Standalone scene input must be an object: " + path.string());
    }
    if (const auto type = reader.getNode("fileType"); type &&
        (!type->isString() || type->stringValue() != "bg2e::scene"))
    {
        throw std::invalid_argument("Unsupported standalone scene fileType: " + path.string());
    }
    if (!reader.getArray("scene"))
    {
        if (reader.getNode("scene") || !allowNode || reader.getString("type").value_or("") != "Node")
        {
            throw std::invalid_argument("Standalone input requires a scene array or permitted Node: " + path.string());
        }
        document = json::JSON(json::JsonObject{{"scene",json::JSON(json::JsonList{document})}});
    }
    // Reuse the shared schema/factory path, including warning/skip policy and
    // custom component attributes. No sceneHasCamera/production creator call.
    return scene::Scene::deserialize(document,path.parent_path());
}

void removeHelpers(scene::Node& node)
{
    const auto components = node.orderedComponents();
    for (const auto& component : components)
    {
        const auto drawable = std::dynamic_pointer_cast<scene::DrawableComponent>(component);
        if (drawable && !drawable->isSceneGeometry())
        {
            node.removeComponent(component);
        }
    }
    for (const auto& child : node.children())
    {
        removeHelpers(*child);
    }
}

std::shared_ptr<draw::Drawable> readDrawable(const std::filesystem::path& path, scene::Node* modelNode = nullptr)
{
    auto drawable = std::make_shared<draw::Drawable>();
    drawable->setName(path.stem().string());
    const auto type = extension(path);
    if (type == ".bg2" || type == ".vwglb")
    {
        const std::unique_ptr<db::Bg2Mesh> model(db::loadMeshBg2(path));
        if (!model || !model->mesh || model->materials.size() != model->mesh->submeshes.size() ||
            model->submeshNames.size() != model->materials.size())
        {
            throw std::invalid_argument("Invalid standalone BG2 mesh/material data: " + path.string());
        }
        drawable->setMesh(std::make_shared<draw::Drawable::Mesh>(*model->mesh));
        for (uint32_t i = 0; i < model->materials.size(); ++i)
        {
            const auto& material = model->materials[i];
            drawable->setMaterial(draw::cloneMaterialAttributes(material),i);
            drawable->setSubmeshName(model->submeshNames[i],i);
            drawable->setSubmeshGroupName(material.groupName(),i);
            drawable->setSubmeshVisibility(material.visible(),i);
        }
        if (modelNode && model->inputJoint)
        {
            auto joint = std::make_shared<scene::InputChainJointComponent>();
            joint->setJoint(*model->inputJoint);
            modelNode->addComponent(joint);
        }
        if (modelNode && model->outputJoint)
        {
            auto joint = std::make_shared<scene::OutputChainJointComponent>();
            joint->setJoint(*model->outputJoint);
            modelNode->addComponent(joint);
        }
    }
    else if (type == ".obj")
    {
        const auto model = db::readObj(path);
        if (!model || !model->mesh || model->materials.size() != model->mesh->submeshes.size() ||
            model->submeshNames.size() != model->materials.size() || model->submeshGroups.size() != model->materials.size())
        {
            throw std::invalid_argument("Invalid standalone OBJ mesh/material data: " + path.string());
        }
        drawable->setMesh(std::make_shared<draw::Drawable::Mesh>(*model->mesh));
        for (uint32_t i = 0; i < model->materials.size(); ++i)
        {
            drawable->setMaterial(draw::cloneMaterialAttributes(model->materials[i]),i);
            drawable->setSubmeshName(model->submeshNames[i],i);
            drawable->setSubmeshGroupName(model->submeshGroups[i],i);
            drawable->setSubmeshVisibility(model->materials[i].visible(),i);
        }
    }
    else
    {
        throw std::invalid_argument("Standalone Drawable source must be BG2/VWGLB or OBJ: " + path.string());
    }
    return drawable;
}

void readTargetDrawables(scene::Node& node, draw::Engine& engine, bool allowGpu)
{
    for (const auto& entry : node.orderedComponents())
    {
        auto* component = dynamic_cast<scene::DrawableComponent*>(entry.get());
        if (!component || !component->isSceneGeometry())
        {
            continue;
        }

        if (!component->drawable())
        {
            const auto path = inputPath(component->resourcePath());
            // Preserve reference display names without duplicating schema parsing.
            const auto reference = component->serializeReferences(path.parent_path());
            const auto name = json::ObjectReader(reference).getString("resourceName");
            const auto type = extension(path);
            if (type == ".bg2" || type == ".vwglb" || type == ".obj")
            {
                auto drawable = readDrawable(path);
                if (name)
                {
                    drawable->setName(*name);
                }
                component->setDrawable(std::move(drawable));
            }
            else if (allowGpu)
            {
                // Registered custom components retain their virtual resource hook.
                // A custom CPU-only route must supply geometry during deserialize.
                component->loadResources(engine);
            }
            else
            {
                throw std::invalid_argument("Custom target requires CPU geometry or GPU realization: " + path.string());
            }
        }
    }
    for (const auto& child : node.children())
    {
        readTargetDrawables(*child,engine,allowGpu);
    }
}

void collectTargets(const std::shared_ptr<scene::Node>& node, const std::filesystem::path& source,
    const std::string& identity, std::vector<StandaloneBakeSceneAssembler::Target>& targets)
{
    if (node->disabled())
    {
        return;
    }
    if (auto* component = node->drawable())
    {
        const auto drawable = component->drawable();
        if (!drawable || !drawable->mesh())
        {
            throw std::invalid_argument("Standalone target has no CPU draw geometry: " + identity);
        }
        const auto path = component->resourcePath().empty() ? source : inputPath(component->resourcePath());
        targets.push_back({node,path,identity,{},std::nullopt});
    }
    for (size_t i = 0; i < node->children().size(); ++i)
    {
        const auto& child = node->children()[i];
        collectTargets(child,source,identity+"/"+segment(child->name(),i),targets);
    }
}

void validateTransform(const glm::mat4& matrix)
{
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!std::isfinite(matrix[column][row]))
            {
                throw std::invalid_argument("Standalone scene has a nonfinite transform");
            }
        }
    }
    const auto determinant = glm::determinant(glm::mat3(matrix));
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
        matrix[0][3] != 0 || matrix[1][3] != 0 || matrix[2][3] != 0 || matrix[3][3] != 1)
    {
        throw std::invalid_argument("Standalone bake requires finite nonsingular affine transforms");
    }
}

void collectDependencies(scene::Node& node, std::set<std::filesystem::path>& paths,
    std::unordered_set<const scene::Node*>& nodes, std::unordered_set<std::string>& identifiers,
    uint32_t& enabledLights, bool enabled = true)
{
    if (!nodes.insert(&node).second || node.identifier().empty() || !identifiers.insert(node.identifier()).second)
    {
        throw std::invalid_argument("Standalone scene has duplicate node ownership/identities");
    }
    enabled = enabled && node.enabled();
    if (enabled)
    {
        if (node.drawable())
        {
            validateTransform(node.worldMatrix());
        }
        if (auto* light = node.light(); light && light->light().type() != base::Light::TypeDisabled)
        {
            if (++enabledLights > draw::MaxLights)
            {
                throw std::length_error("Standalone sources exceed the eight enabled light ABI");
            }
            const auto world = node.worldMatrix();
            const auto position = world * glm::vec4(0,0,0,1);
            const auto direction = glm::mat3(world) * glm::vec3(0,0,1);
            const float length = glm::length(direction);
            if (!std::isfinite(length) || length < 1e-12f ||
                !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
            {
                throw std::invalid_argument("Standalone light position/direction must be finite and nonzero");
            }
            const auto& value = light->light();
            if (value.type() != base::Light::TypeOmni && value.type() != base::Light::TypeSpot &&
                value.type() != base::Light::TypeDirectional && value.type() != base::Light::TypeSun)
            {
                throw std::invalid_argument("Unsupported standalone source light type");
            }
            const auto& color = value.color();
            for (const auto scalar : {color.r,color.g,color.b,color.a,value.intensity(),value.spotAngle(),
                value.spotCutoff(),value.sourceSize()})
            {
                if (!std::isfinite(scalar))
                {
                    throw std::invalid_argument("Standalone source light values must be finite");
                }
            }
        }
    }
    if (auto* environment = node.environment(); environment && !environment->environmentImage().empty())
    {
        paths.insert(std::filesystem::canonical(inputPath(environment->environmentImage())));
    }
    // Traverse all authored references, even disabled sources, for input protection.
    for (const auto& entry : node.orderedComponents())
    {
        const auto* component = dynamic_cast<const scene::DrawableComponent*>(entry.get());
        if (!component || !component->isSceneGeometry())
        {
            continue;
        }
        if (!component->resourcePath().empty())
        {
            paths.insert(std::filesystem::canonical(inputPath(component->resourcePath())));
        }
        const auto drawable = component->drawable();
        if (!drawable)
        {
            throw std::invalid_argument("Standalone authored Drawable has unresolved resources");
        }
        for (uint32_t i = 0; i < drawable->submeshesCount(); ++i)
        {
            const auto& material = drawable->material(i);
            for (const auto& texture : {material.albedoTexture(),material.metalnessTexture(),material.roughnessTexture(),
                material.normalTexture(),material.aoTexture(),material.lightEmissionTexture()})
            {
                if (texture && texture->type() == base::Texture::TypeFilesystem && !texture->imageFilePath().empty())
                {
                    paths.insert(std::filesystem::canonical(inputPath(texture->imageFilePath())));
                }
            }
        }
    }
    for (const auto& child : node.children())
    {
        if (!child || child->parent() != &node)
        {
            throw std::invalid_argument("Standalone scene has invalid child ownership");
        }
        collectDependencies(*child,paths,nodes,identifiers,enabledLights,enabled);
    }
}
}

StandaloneBakeSceneAssembler::StandaloneBakeSceneAssembler(draw::Engine& engine)
    : _engine(&engine), _deviceIdentity(engine.device() ? engine.device()->timingIdentity() : 0)
{
    requireEngine();
}

void StandaloneBakeSceneAssembler::requireEngine() const
{
    if (!_engine->device() || !_engine->device()->isValid() || !_engine->offscreenSurface() ||
        _engine->device()->timingIdentity() != _deviceIdentity)
    {
        throw std::logic_error("Standalone assembly requires the original initialized windowless draw Engine");
    }
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assembleModel(
    const std::filesystem::path& contextJson, const std::filesystem::path& model,
    std::optional<geo::Uv2AtlasOptions> uv2Options, bool loadTargetGpuResources) const
{
    requireEngine();
    const auto contextPath = inputPath(contextJson);
    const auto modelPath = inputPath(model);
    auto root = std::make_shared<scene::Node>(modelPath.stem().string());
    auto drawable = readDrawable(modelPath,root.get());
    auto component = std::make_shared<scene::DrawableComponent>(std::move(drawable));
    component->setResourcePath(modelPath);
    root->addComponent(component);
    root->addComponent(new scene::TransformComponent());
    return assemble(contextPath,modelPath,std::move(root),uv2Options,loadTargetGpuResources);
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assemblePrefab(
    const std::filesystem::path& contextJson, const std::filesystem::path& prefabJson,
    std::optional<geo::Uv2AtlasOptions> uv2Options, bool loadTargetGpuResources) const
{
    requireEngine();
    const auto contextPath = inputPath(contextJson);
    const auto prefabPath = inputPath(prefabJson);
    auto prefab = readScene(prefabPath,true);
    auto root = prefab->releaseSceneRoot();
    // Only the loader's synthetic wrapper is eligible for removal, exactly once.
    if (root->children().size() == 1 && root->components().empty())
    {
        auto child = root->children().front();
        root->removeChild(child);
        root = std::move(child);
    }
    removeHelpers(*root);
    return assemble(contextPath,prefabPath,std::move(root),uv2Options,loadTargetGpuResources);
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assemble(
    const std::filesystem::path& contextPath, const std::filesystem::path& input,
    std::shared_ptr<scene::Node> targetRoot, std::optional<geo::Uv2AtlasOptions> uv2Options,
    bool loadTargetGpuResources) const
{
    base::CpuProfileScope profile("bake.standalone.assembleScene");
    requireEngine();
    _engine->device()->waitIdle();
    auto context = readScene(contextPath,false);
    removeHelpers(*context->rootNode());
    context->rootNode()->loadResources(*_engine);
    removeHelpers(*context->rootNode());
    if (!targetRoot || targetRoot->parent())
    {
        throw std::invalid_argument("Standalone target root must be detached before assembly");
    }
    context->rootNode()->addChild(targetRoot);
    readTargetDrawables(*targetRoot,*_engine,loadTargetGpuResources);
    removeHelpers(*targetRoot);
    Assembly result;
    result.scene = context;
    result.targetRoot = std::move(targetRoot);
    result.contextSourcePath = contextPath;
    result.inputSourcePath = input;
    collectTargets(result.targetRoot,input,segment(result.targetRoot->name(),0),result.targets);
    if (result.targets.empty())
    {
        throw std::invalid_argument("Standalone input has no enabled scene-geometry targets: " + input.string());
    }
    std::vector<std::shared_ptr<draw::Drawable>> replacements;
    std::vector<std::shared_ptr<const draw::Drawable::Mesh>> meshes;
    for (auto& target : result.targets)
    {
        try
        {
            const auto drawable = target.node->drawable()->drawable();
            auto replacement = drawable;
            if (uv2Options)
            {
                if (drawable->isLoaded())
                {
                    auto generated = generateUv2AndReload(drawable,*uv2Options);
                    replacement = std::move(generated.drawable);
                    target.generatedUv2 = generated.atlas;
                }
                else
                {
                    replacement = drawable->clone();
                    auto mesh = std::make_shared<draw::Drawable::Mesh>(*drawable->mesh());
                    geo::GenerateUv2AtlasModifier modifier(mesh.get(),*uv2Options);
                    modifier.apply();
                    target.generatedUv2 = modifier.result();
                    replacement->setMesh(std::move(mesh));
                }
            }
            for (uint32_t i = 0; i < replacement->submeshesCount(); ++i)
            {
                if (replacement->material(i).isTransparent())
                {
                    throw std::invalid_argument("Transparent materials are not supported baked-map receivers");
                }
                validateTransform(target.node->worldMatrix()*replacement->submeshTransform(i));
            }
            target.uv2 = geo::UvAtlasValidator::validate(*replacement->mesh(),1,geo::UvAtlasOverlapPolicy::Warn);
            buildUvSurfaceMesh(*replacement->mesh()); // Finite geometry/normals, ranges and chart validation; logs overlaps.
            meshes.push_back(replacement->mesh());
            replacements.push_back(std::move(replacement));
        }
        catch (const std::exception& error)
        {
            throw std::runtime_error("Standalone target '"+target.outputIdentity+"' from '"+
                target.inputSourcePath.string()+"': "+error.what());
        }
    }
    // All replacements validate before any candidate component adopts one. Nothing
    // here is attached to the caller's context or published as an Assembly yet.
    for (size_t i = 0; i < result.targets.size(); ++i)
    {
        result.targets[i].node->drawable()->setDrawable(replacements[i]);
    }
    if (loadTargetGpuResources)
    {
        result.targetRoot->loadResources(*_engine);
        removeHelpers(*result.targetRoot);
        for (size_t i = 0; i < result.targets.size(); ++i)
        {
            auto* component = result.targets[i].node->drawable();
            const auto drawable = component ? component->drawable() : nullptr;
            if (drawable != replacements[i] || drawable->mesh() != meshes[i] ||
                !drawable->isLoaded() || drawable->engine() != _engine)
            {
                throw std::logic_error("Resource loading replaced/incompletely realized a prepared standalone target");
            }
            buildUvSurfaceMesh(*drawable->mesh());
        }
        draw::RenderQueue queue;
        draw::RenderQueueVisitor visitor;
        visitor.enqueue(context->rootNode(),*_engine,queue);
    }
    context->updateAll();
    std::set<std::filesystem::path> paths{std::filesystem::canonical(contextPath),std::filesystem::canonical(input)};
    std::unordered_set<const scene::Node*> nodes;
    std::unordered_set<std::string> identifiers;
    uint32_t lights = 0;
    collectDependencies(*context->rootNode(),paths,nodes,identifiers,lights);
    result.sourcePaths.assign(paths.begin(),paths.end());
    return result;
}
}
