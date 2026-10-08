#include <bg2e/render/StandaloneBakeSceneAssembler.hpp>

#include <bg2e/base/Log.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/scene.hpp>
#include <bg2e/db/scene_bg2.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TransformComponent.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace bg2e {
namespace render {
namespace {

using JsonNodes = std::vector<std::shared_ptr<json::JsonNode>>;

struct ParsedSceneInput {
    std::shared_ptr<json::JsonNode> document;
    JsonNodes roots;
    bool isSceneDocument = false;
};

struct DrawableTarget {
    std::shared_ptr<scene::Node> node;
    std::shared_ptr<scene::Drawable> drawable;
    std::string outputIdentity;
};

std::filesystem::path absoluteInputPath(
    const std::filesystem::path& path,
    const std::string& kind)
{
    if (path.empty())
    {
        throw std::invalid_argument("StandaloneBakeSceneAssembler: " + kind + " path must not be empty");
    }

    std::error_code error;
    auto result = std::filesystem::absolute(path, error);
    if (error)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: could not resolve " + kind + " path '" +
            path.string() + "': " + error.message());
    }
    result = result.lexically_normal();
    if (!std::filesystem::is_regular_file(result, error) || error)
    {
        throw std::invalid_argument("StandaloneBakeSceneAssembler: " + kind + " file does not exist: '" +
            result.string() + "'");
    }
    return result;
}

std::shared_ptr<json::JsonNode> parseJsonFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: could not open JSON file '" + path.string() + "'");
    }

    std::string contents;
    contents.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    try
    {
        json::JsonParser parser(contents);
        auto document = parser.parse();
        if (!document)
        {
            throw std::runtime_error("parser returned no JSON value");
        }
        return document;
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: invalid JSON file '" + path.string() +
            "': " + error.what());
    }
}

void requireRegularFile(const std::filesystem::path& path, const std::string& role)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: missing " + role + " file '" +
            path.string() + "'");
    }
}

void validateTextureFile(const std::shared_ptr<base::Texture>& texture,
                         const std::filesystem::path& owningAsset)
{
    if (!texture || texture->type() != base::Texture::TypeFilesystem || texture->imageFilePath().empty())
    {
        return;
    }

    auto texturePath = std::filesystem::path(texture->imageFilePath());
    if (texturePath.is_relative())
    {
        texturePath = owningAsset.parent_path() / texturePath;
    }
    requireRegularFile(texturePath.lexically_normal(), "material texture referenced by");
}

void validateBg2Asset(const std::filesystem::path& path)
{
    std::unique_ptr<db::Bg2Mesh> mesh;
    try
    {
        mesh.reset(db::loadMeshBg2(path));
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: invalid .bg2 asset '" + path.string() +
            "': " + error.what());
    }
    if (!mesh || !mesh->mesh || mesh->mesh->vertices.empty() || mesh->mesh->indices.empty() ||
        mesh->mesh->submeshes.empty())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: .bg2 asset contains no usable mesh: '" +
            path.string() + "'");
    }
    if (mesh->materials.size() != mesh->mesh->submeshes.size())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: material/submesh count mismatch in '" +
            path.string() + "'");
    }

    uint64_t nextIndex = 0;
    for (size_t submeshIndex = 0; submeshIndex < mesh->mesh->submeshes.size(); ++submeshIndex)
    {
        const auto& submesh = mesh->mesh->submeshes[submeshIndex];
        const uint64_t end = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (submesh.firstIndex != nextIndex || submesh.indexCount == 0 ||
            submesh.indexCount % 3 != 0 || end > mesh->mesh->indices.size())
        {
            throw std::runtime_error("StandaloneBakeSceneAssembler: invalid triangle submesh range " +
                std::to_string(submeshIndex) + " in '" + path.string() + "'");
        }
        for (uint64_t index = submesh.firstIndex; index < end; ++index)
        {
            if (mesh->mesh->indices[static_cast<size_t>(index)] >= mesh->mesh->vertices.size())
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: vertex index out of range in '" +
                    path.string() + "'");
            }
        }
        nextIndex = end;
    }
    if (nextIndex != mesh->mesh->indices.size())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: submesh ranges do not cover the index buffer in '" +
            path.string() + "'");
    }

    for (const auto& material : mesh->materials)
    {
        validateTextureFile(material.albedoTexture(), path);
        validateTextureFile(material.metalnessTexture(), path);
        validateTextureFile(material.roughnessTexture(), path);
        validateTextureFile(material.normalTexture(), path);
        validateTextureFile(material.aoTexture(), path);
        validateTextureFile(material.lightEmissionTexture(), path);
    }
}

std::filesystem::path drawableAssetPath(
    const std::filesystem::path& basePath,
    const std::string& name)
{
    auto path = basePath / name;
    path.replace_extension(".bg2");
    return path.lexically_normal();
}

struct JsonValidationSummary {
    size_t componentCount = 0;
    std::vector<std::filesystem::path> drawableSourcePaths;
};

size_t countLoadedDrawables(scene::Node* node);
size_t countLoadedComponents(scene::Node* node);

JsonValidationSummary validateSerializedNode(
    const std::shared_ptr<json::JsonNode>& nodeData,
    const std::filesystem::path& basePath,
    const std::filesystem::path& ownerPath,
    const std::string& nodePath)
{
    json::ObjectReader nodeReader(nodeData);
    if (!nodeReader.isValid())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: malformed node at " + nodePath +
            " in '" + ownerPath.string() + "'");
    }

    JsonValidationSummary result;
    if (auto components = nodeReader.getArray("components"))
    {
        for (size_t index = 0; index < components->size(); ++index)
        {
            auto component = components->getObject(index);
            if (!component)
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: malformed component " +
                    std::to_string(index) + " at " + nodePath + " in '" + ownerPath.string() + "'");
            }
            const auto type = component->getString("type");
            if (!type || type->empty() || !scene::ComponentFactoryRegistry::get().contains(*type))
            {
                bg2e_log_warning << "StandaloneBakeSceneAssembler: skipping unknown or missing component type '"
                    << (type ? *type : std::string{}) << "' at " << nodePath
                    << " in '" << ownerPath.string() << "'" << bg2e_log_end;
                continue;
            }
            ++result.componentCount;

            if (*type == "Drawable")
            {
                const auto name = component->getString("name");
                if (!name || name->empty())
                {
                    throw std::runtime_error("StandaloneBakeSceneAssembler: Drawable at " + nodePath +
                        " has no asset name in '" + ownerPath.string() + "'");
                }
                const auto drawablePath = drawableAssetPath(basePath, *name);
                requireRegularFile(drawablePath, "Drawable referenced by " + nodePath + " in");
                validateBg2Asset(drawablePath);
                result.drawableSourcePaths.push_back(drawablePath);
            }
            else if (*type == "Environment")
            {
                if (auto textureName = component->getString("equirectangularTexture");
                    textureName && !textureName->empty())
                {
                    requireRegularFile(basePath / *textureName,
                        "environment texture referenced by " + nodePath + " in");
                }
            }
        }
    }
    else if (!nodeReader.isUndefined("components") && !nodeReader.isNull("components"))
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: components must be an array at " +
            nodePath + " in '" + ownerPath.string() + "'");
    }

    if (auto children = nodeReader.getArray("children"))
    {
        for (size_t index = 0; index < children->size(); ++index)
        {
            auto child = children->getObject(index);
            if (!child)
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: malformed child " +
                    std::to_string(index) + " at " + nodePath + " in '" + ownerPath.string() + "'");
            }
            const auto childPath = nodePath + "/children[" + std::to_string(index) + "]";
            const auto childSummary = validateSerializedNode(
                child->node(), basePath, ownerPath, childPath);
            result.componentCount += childSummary.componentCount;
            result.drawableSourcePaths.insert(result.drawableSourcePaths.end(),
                childSummary.drawableSourcePaths.begin(), childSummary.drawableSourcePaths.end());
        }
    }
    else if (!nodeReader.isUndefined("children") && !nodeReader.isNull("children"))
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: children must be an array at " +
            nodePath + " in '" + ownerPath.string() + "'");
    }

    return result;
}

ParsedSceneInput parseSceneInput(
    const std::filesystem::path& path,
    bool allowSingleNode)
{
    ParsedSceneInput result;
    result.document = parseJsonFile(path);

    json::ObjectReader reader(result.document);
    if (!reader.isValid())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: JSON root must be an object in '" +
            path.string() + "'");
    }

    if (auto sceneNodes = reader.getArray("scene"))
    {
        result.isSceneDocument = true;
        for (size_t index = 0; index < sceneNodes->size(); ++index)
        {
            auto node = sceneNodes->getObject(index);
            if (!node)
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: malformed scene node " +
                    std::to_string(index) + " in '" + path.string() + "'");
            }
            result.roots.push_back(node->node());
        }
        return result;
    }

    if (reader.isObject("scene"))
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: scene must be an array in '" +
            path.string() + "'");
    }

    if (allowSingleNode && reader.getString("type").value_or("") == "Node")
    {
        result.roots.push_back(result.document);
        return result;
    }

    throw std::runtime_error("StandaloneBakeSceneAssembler: expected a scene document" +
        std::string(allowSingleNode ? " or serialized Node" : "") + " in '" + path.string() + "'");
}

std::shared_ptr<scene::Scene> loadContextScene(
    const std::filesystem::path& path,
    Engine* engine)
{
    const auto parsed = parseSceneInput(path, false);
    JsonValidationSummary summary;
    for (size_t index = 0; index < parsed.roots.size(); ++index)
    {
        const auto nodeSummary = validateSerializedNode(
            parsed.roots[index], path.parent_path(), path,
            "scene[" + std::to_string(index) + "]");
        summary.componentCount += nodeSummary.componentCount;
        summary.drawableSourcePaths.insert(summary.drawableSourcePaths.end(),
            nodeSummary.drawableSourcePaths.begin(), nodeSummary.drawableSourcePaths.end());
    }

    auto loadedScene = db::loadScene(path, *engine);
    if (!loadedScene || !loadedScene->rootNode())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: failed to load context scene '" +
            path.string() + "'");
    }
    const auto loadedDrawableCount = countLoadedDrawables(loadedScene->rootNode());
    if (loadedDrawableCount != summary.drawableSourcePaths.size())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: context scene '" + path.string() +
            "' loaded " + std::to_string(loadedDrawableCount) + " of " +
            std::to_string(summary.drawableSourcePaths.size()) + " declared Drawable components");
    }
    if (countLoadedComponents(loadedScene->rootNode()) != summary.componentCount)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: context scene contains components that failed to load: '" +
            path.string() + "'");
    }
    return loadedScene;
}

struct LoadedPrefab {
    std::shared_ptr<scene::Scene> scene;
    size_t componentCount = 0;
    std::vector<std::filesystem::path> drawableSourcePaths;
};

std::shared_ptr<scene::Drawable> loadCpuDrawable(const std::filesystem::path& path)
{
    std::unique_ptr<db::Bg2Mesh> source(db::loadMeshBg2(path));
    if (!source || !source->mesh)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: failed to load CPU mesh '" + path.string() + "'");
    }

    auto drawable = std::make_shared<scene::Drawable>();
    drawable->setName(path.stem().string());
    drawable->setMesh(source->mesh);
    for (uint32_t index = 0; index < source->materials.size(); ++index)
    {
        const auto& material = source->materials[index];
        drawable->setMaterial(material, index);
        drawable->setSubmeshName(material.name(), index);
        drawable->setSubmeshGroupName(material.groupName(), index);
        drawable->setSubmeshVisibility(material.visible(), index);
    }
    return drawable;
}

std::shared_ptr<scene::Node> deserializeNodeWithCpuDrawables(
    const std::shared_ptr<json::JsonNode>& nodeData,
    const std::filesystem::path& basePath,
    const std::filesystem::path& ownerPath,
    const std::string& nodePath,
    Engine* engine,
    std::vector<std::filesystem::path>& drawableSourcePaths)
{
    json::ObjectReader reader(nodeData);
    if (!reader.isValid())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: malformed CPU-loaded prefab node at " +
            nodePath + " in '" + ownerPath.string() + "'");
    }

    auto node = std::make_shared<scene::Node>(reader.getString("name").value_or(""));
    if (auto enabled = reader.getBool("enabled")) node->setEnabled(*enabled);
    if (auto steady = reader.getBool("steady")) node->setSteady(*steady);

    if (auto components = reader.getArray("components"))
    {
        for (size_t index = 0; index < components->size(); ++index)
        {
            auto componentReader = components->getObject(index);
            if (!componentReader)
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: malformed component " +
                    std::to_string(index) + " at " + nodePath + " in '" + ownerPath.string() + "'");
            }
            auto componentData = componentReader->node();
            const auto type = componentReader->getString("type").value_or("");
            if (type == "Drawable")
            {
                const auto assetName = componentReader->getString("name");
                if (!assetName || assetName->empty())
                {
                    throw std::runtime_error("StandaloneBakeSceneAssembler: Drawable at " + nodePath +
                        " has no asset name in '" + ownerPath.string() + "'");
                }
                const auto drawablePath = drawableAssetPath(basePath, *assetName);
                auto drawable = loadCpuDrawable(drawablePath);
                node->addComponent(std::make_shared<scene::DrawableComponent>(drawable));
                drawableSourcePaths.push_back(drawablePath);
                continue;
            }

            auto* component = scene::ComponentFactoryRegistry::get().create(
                componentData, basePath, *engine);
            if (!component)
            {
                bg2e_log_warning << "StandaloneBakeSceneAssembler: skipping component '" << type
                    << "' that failed to load at " << nodePath
                    << " in '" << ownerPath.string() << "'" << bg2e_log_end;
                continue;
            }
            node->addComponent(component);
        }
    }

    if (auto children = reader.getArray("children"))
    {
        for (size_t index = 0; index < children->size(); ++index)
        {
            auto child = children->getObject(index);
            if (!child)
            {
                throw std::runtime_error("StandaloneBakeSceneAssembler: malformed child " +
                    std::to_string(index) + " at " + nodePath + " in '" + ownerPath.string() + "'");
            }
            node->addChild(deserializeNodeWithCpuDrawables(
                child->node(), basePath, ownerPath,
                nodePath + "/children[" + std::to_string(index) + "]",
                engine, drawableSourcePaths));
        }
    }
    return node;
}

LoadedPrefab loadPrefabSceneCpu(
    const ParsedSceneInput& parsed,
    const std::filesystem::path& path,
    Engine* engine)
{
    auto root = std::make_shared<scene::Node>("scene root");
    std::vector<std::filesystem::path> drawableSourcePaths;
    for (size_t index = 0; index < parsed.roots.size(); ++index)
    {
        root->addChild(deserializeNodeWithCpuDrawables(
            parsed.roots[index], path.parent_path(), path,
            parsed.isSceneDocument ? "scene[" + std::to_string(index) + "]" : "root",
            engine, drawableSourcePaths));
    }
    if (drawableSourcePaths.empty())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab contains no Drawable components: '" +
            path.string() + "'");
    }

    auto loadedScene = std::make_shared<scene::Scene>();
    loadedScene->setSceneRoot(root);
    return {
        std::move(loadedScene),
        countLoadedComponents(root.get()),
        std::move(drawableSourcePaths)
    };
}

LoadedPrefab loadPrefabScene(
    const std::filesystem::path& path,
    Engine* engine,
    bool loadTargetGpuResources)
{
    const auto parsed = parseSceneInput(path, true);
    if (parsed.roots.empty())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab contains no root nodes: '" +
            path.string() + "'");
    }

    std::vector<std::filesystem::path> drawableSourcePaths;
    size_t componentCount = 0;
    for (size_t index = 0; index < parsed.roots.size(); ++index)
    {
        const auto nodeSummary = validateSerializedNode(
            parsed.roots[index], path.parent_path(), path,
            parsed.isSceneDocument ? "scene[" + std::to_string(index) + "]" : "root");
        componentCount += nodeSummary.componentCount;
        drawableSourcePaths.insert(drawableSourcePaths.end(),
            nodeSummary.drawableSourcePaths.begin(), nodeSummary.drawableSourcePaths.end());
    }
    if (drawableSourcePaths.empty())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab contains no Drawable components: '" +
            path.string() + "'");
    }

    if (!loadTargetGpuResources)
    {
        return loadPrefabSceneCpu(parsed, path, engine);
    }

    std::shared_ptr<scene::Scene> loadedScene;
    if (parsed.isSceneDocument)
    {
        loadedScene = db::loadScene(path, *engine);
    }
    else
    {
        using namespace json;
        auto wrappedDocument = JSON(JsonObject{
            { "scene", JSON(JsonList{ parsed.roots.front() }) }
        });
        loadedScene = scene::Scene::deserialize(
            wrappedDocument, path.parent_path(), *engine);
    }

    if (!loadedScene || !loadedScene->rootNode())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: failed to load prefab '" + path.string() + "'");
    }
    return { std::move(loadedScene), componentCount, std::move(drawableSourcePaths) };
}

size_t countLoadedDrawables(scene::Node* node)
{
    if (!node)
    {
        return 0;
    }
    size_t count = node->drawable() ? 1 : 0;
    for (const auto& child : node->children())
    {
        count += countLoadedDrawables(child.get());
    }
    return count;
}

size_t countLoadedComponents(scene::Node* node)
{
    if (!node)
    {
        return 0;
    }
    size_t count = node->orderedComponents().size();
    for (const auto& child : node->children())
    {
        count += countLoadedComponents(child.get());
    }
    return count;
}

std::string outputSegment(const std::string& name, size_t siblingIndex)
{
    std::string safeName;
    safeName.reserve(name.size());
    for (const unsigned char character : name)
    {
        if (std::isalnum(character) || character == '_' || character == '-')
        {
            safeName.push_back(static_cast<char>(character));
        }
        else
        {
            safeName.push_back('_');
        }
    }
    if (safeName.empty())
    {
        safeName = "node";
    }
    return safeName + "-" + std::to_string(siblingIndex);
}

void collectSceneTargets(
    scene::Node* node,
    scene::Node* targetRoot,
    const std::string& relativeIdentity,
    bool insideTarget,
    std::unordered_set<const scene::Node*>& seenNodes,
    std::unordered_set<std::string>& seenIdentifiers,
    std::vector<DrawableTarget>& targets,
    const std::filesystem::path& inputPath,
    bool requireGpuLoaded)
{
    if (!node)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: scene contains a null node");
    }
    if (!seenNodes.insert(node).second)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: duplicate node attachment under '" +
            node->name() + "' while assembling '" + inputPath.string() + "'");
    }
    if (node->identifier().empty() || !seenIdentifiers.insert(node->identifier()).second)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: duplicate or empty node identity at '" +
            node->name() + "' while assembling '" + inputPath.string() + "'");
    }

    const bool isTargetRoot = node == targetRoot;
    const bool inTargetSubtree = insideTarget || isTargetRoot;
    std::string nodeIdentity = relativeIdentity;
    if (isTargetRoot)
    {
        nodeIdentity = outputSegment(node->name(), 0);
    }

    if (inTargetSubtree)
    {
        if (auto* component = node->drawable())
        {
            auto drawable = component->drawable();
            if (drawable)
            {
                if ((requireGpuLoaded && !drawable->isLoaded()) || !drawable->mesh())
                {
                    throw std::runtime_error("StandaloneBakeSceneAssembler: target Drawable is unavailable at '" +
                        nodeIdentity + "' from '" + inputPath.string() + "'");
                }
                targets.push_back({ node->shared_from_this(), std::move(drawable), nodeIdentity });
            }
        }
    }

    const auto& children = node->children();
    for (size_t index = 0; index < children.size(); ++index)
    {
        const auto& child = children[index];
        if (!child || child->parent() != node)
        {
            throw std::runtime_error("StandaloneBakeSceneAssembler: invalid child ownership below '" +
                node->name() + "' while assembling '" + inputPath.string() + "'");
        }
        std::string childIdentity = relativeIdentity;
        if (inTargetSubtree)
        {
            childIdentity = nodeIdentity + "/" + outputSegment(child->name(), index);
        }
        collectSceneTargets(child.get(), targetRoot, childIdentity, inTargetSubtree,
                            seenNodes, seenIdentifiers, targets, inputPath, requireGpuLoaded);
    }
}

std::vector<DrawableTarget> collectTargets(
    scene::Node* contextRoot,
    scene::Node* targetRoot,
    const std::filesystem::path& inputPath,
    bool requireGpuLoaded)
{
    if (!contextRoot || !targetRoot || targetRoot->parent() != contextRoot ||
        targetRoot->sceneRoot() != contextRoot)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: target subtree was not attached to the context root for '" +
            inputPath.string() + "'");
    }

    std::vector<DrawableTarget> targets;
    std::unordered_set<const scene::Node*> seenNodes;
    std::unordered_set<std::string> seenIdentifiers;
    collectSceneTargets(contextRoot, targetRoot, {}, false,
                        seenNodes, seenIdentifiers, targets, inputPath, requireGpuLoaded);
    if (targets.empty())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: target input contains no standard Drawable nodes: '" +
            inputPath.string() + "'");
    }
    return targets;
}

void generateUv2AndReload(
    Engine* engine,
    std::vector<DrawableTarget>& targets,
    const std::optional<geo::Uv2AtlasOptions>& options,
    const std::filesystem::path& inputPath,
    bool targetGpuResourcesLoaded)
{
    if (!options)
    {
        return;
    }

    if (targetGpuResourcesLoaded)
    {
        // Immediate-load mode modifies CPU meshes only after stopping GPU use,
        // then reloads raster resources and BLASes before the caller builds its TLAS.
        engine->device().waitIdle();
    }
    for (auto& target : targets)
    {
        try
        {
            auto mesh = target.drawable->mesh();
            if (!mesh)
            {
                throw std::runtime_error("Drawable has no CPU mesh");
            }
            geo::GenerateUv2AtlasModifier modifier(mesh.get(), *options);
            modifier.apply();
        }
        catch (const std::exception& error)
        {
            throw std::runtime_error("StandaloneBakeSceneAssembler: UV2 generation failed for target '" +
                target.outputIdentity + "' from '" + inputPath.string() + "': " + error.what());
        }
    }

    if (!targetGpuResourcesLoaded)
    {
        return;
    }

    try
    {
        for (auto& target : targets)
        {
            target.drawable->reload();
        }
        engine->device().waitIdle();
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: GPU reload after UV2 generation failed for '" +
            inputPath.string() + "': " + error.what());
    }
}

std::vector<StandaloneBakeSceneAssembler::Target> makeAssemblyTargets(
    const std::vector<DrawableTarget>& targets,
    const std::vector<std::filesystem::path>& inputSourcePaths)
{
    if (targets.size() != inputSourcePaths.size())
    {
        throw std::logic_error("StandaloneBakeSceneAssembler: target/source-path count mismatch");
    }
    std::vector<StandaloneBakeSceneAssembler::Target> result;
    result.reserve(targets.size());
    for (size_t index = 0; index < targets.size(); ++index)
    {
        result.push_back({ targets[index].node, inputSourcePaths[index], targets[index].outputIdentity });
    }
    return result;
}

}

StandaloneBakeSceneAssembler::StandaloneBakeSceneAssembler(Engine* engine)
    : _engine(engine)
{
    if (!_engine)
    {
        throw std::invalid_argument("StandaloneBakeSceneAssembler: engine must not be null");
    }
    if (!_engine->rayTracingSupported())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler requires ray tracing support");
    }
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assembleModel(
    const std::filesystem::path& contextJson,
    const std::filesystem::path& modelBg2,
    std::optional<geo::Uv2AtlasOptions> uv2Options) const
{
    return assembleModel(contextJson, modelBg2, uv2Options, true);
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assembleModel(
    const std::filesystem::path& contextJson,
    const std::filesystem::path& modelBg2,
    std::optional<geo::Uv2AtlasOptions> uv2Options,
    bool loadTargetGpuResources) const
{
    const auto contextPath = absoluteInputPath(contextJson, "context");
    const auto modelPath = absoluteInputPath(modelBg2, "model");
    std::string extension = modelPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (extension != ".bg2")
    {
        throw std::invalid_argument("StandaloneBakeSceneAssembler: model input must be a .bg2 file: '" +
            modelPath.string() + "'");
    }
    validateBg2Asset(modelPath);

    auto contextScene = loadContextScene(contextPath, _engine);
    auto modelRoot = db::loadSceneBg2(modelPath, _engine, loadTargetGpuResources);
    if (!modelRoot)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: failed to load model '" + modelPath.string() + "'");
    }
    if (modelRoot->parent())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: model loader returned an already attached node for '" +
            modelPath.string() + "'");
    }
    auto* transform = modelRoot->transform();
    if (!transform)
    {
        transform = new scene::TransformComponent();
        modelRoot->addComponent(transform);
    }
    transform->setIdentity();
    contextScene->rootNode()->addChild(modelRoot);

    auto targets = collectTargets(
        contextScene->rootNode(), modelRoot.get(), modelPath, loadTargetGpuResources);
    if (targets.size() != 1 || targets.front().node != modelRoot)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: .bg2 model must produce exactly one Drawable target: '" +
            modelPath.string() + "'");
    }
    generateUv2AndReload(_engine, targets, uv2Options, modelPath, loadTargetGpuResources);

    return {
        std::move(contextScene),
        contextPath,
        modelPath,
        makeAssemblyTargets(targets, std::vector<std::filesystem::path>{ modelPath })
    };
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assemblePrefab(
    const std::filesystem::path& contextJson,
    const std::filesystem::path& prefabJson,
    std::optional<geo::Uv2AtlasOptions> uv2Options) const
{
    return assemblePrefab(contextJson, prefabJson, uv2Options, true);
}

StandaloneBakeSceneAssembler::Assembly StandaloneBakeSceneAssembler::assemblePrefab(
    const std::filesystem::path& contextJson,
    const std::filesystem::path& prefabJson,
    std::optional<geo::Uv2AtlasOptions> uv2Options,
    bool loadTargetGpuResources) const
{
    const auto contextPath = absoluteInputPath(contextJson, "context");
    const auto prefabPath = absoluteInputPath(prefabJson, "prefab");

    auto contextScene = loadContextScene(contextPath, _engine);
    auto prefab = loadPrefabScene(prefabPath, _engine, loadTargetGpuResources);
    auto prefabRoot = prefab.scene->rootNode()->shared_from_this();
    if (!prefabRoot || prefabRoot->parent())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab root cannot be attached cleanly from '" +
            prefabPath.string() + "'");
    }

    contextScene->rootNode()->addChild(prefabRoot);
    if (prefabRoot->parent() != contextScene->rootNode() ||
        prefabRoot->sceneRoot() != contextScene->rootNode())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab root attachment failed for '" +
            prefabPath.string() + "'");
    }
    prefab.scene.reset();

    const auto loadedDrawableCount = countLoadedDrawables(prefabRoot.get());
    if (loadedDrawableCount != prefab.drawableSourcePaths.size())
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab '" + prefabPath.string() +
            "' loaded " + std::to_string(loadedDrawableCount) + " of " +
            std::to_string(prefab.drawableSourcePaths.size()) + " declared Drawable components");
    }
    if (countLoadedComponents(prefabRoot.get()) != prefab.componentCount)
    {
        throw std::runtime_error("StandaloneBakeSceneAssembler: prefab '" + prefabPath.string() +
            "' did not load every declared component");
    }

    auto targets = collectTargets(
        contextScene->rootNode(), prefabRoot.get(), prefabPath, loadTargetGpuResources);
    generateUv2AndReload(
        _engine, targets, uv2Options, prefabPath, loadTargetGpuResources);

    return {
        std::move(contextScene),
        contextPath,
        prefabPath,
        makeAssemblyTargets(targets, prefab.drawableSourcePaths)
    };
}

}
}
