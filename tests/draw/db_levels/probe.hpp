#pragma once

#include <bg2e/db/scene.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>
#include <bg2e/json/NodeReader.hpp>

#include <filesystem>
#include <stdexcept>

namespace bg2e::draw::tests {
// The caller supplies an existing disposable directory. No engine is needed.
inline void runSceneRepresentationProbe(const std::filesystem::path& directory)
{
    using namespace json;
    const auto missingMesh = std::filesystem::absolute(directory / "unresolved.bg2").lexically_normal();
    const auto missingEnvironment = std::filesystem::absolute(directory / "unresolved.hdr").lexically_normal();
    if (std::filesystem::exists(missingMesh) || std::filesystem::exists(missingEnvironment))
    {
        throw std::invalid_argument("Representation probe requires absent resource files");
    }
    auto drawableData = JSON(JsonObject{{"type", JSON("Drawable")}, {"name", JSON("unresolved")}, {"priority", JSON(7)}});
    auto environmentData = JSON(JsonObject{{"type", JSON("Environment")}, {"equirectangularTexture", JSON("unresolved.hdr")}});
    auto nodeData = JSON(JsonObject{
        {"name", JSON("reference node")}, {"enabled", JSON(false)}, {"steady", JSON(true)},
        {"components", JSON(JsonList{drawableData, environmentData,
            JSON(JsonObject{{"type", JSON("Transform")}, {"transformMatrix", JSON(glm::mat4{1})}})})},
        {"children", JSON(JsonList{})}});
    auto document = JSON(JsonObject{
        {"fileType", JSON("bg2e::scene")}, {"customDocumentField", JSON("preserved")},
        {"scene", JSON(JsonList{nodeData})}});
    const auto inputPath = directory / "references-input.bg2e";
    db::writeSceneDocument(document, inputPath);
    auto read = db::readSceneDocument(inputPath);
    if (ObjectReader(read).getString("customDocumentField").value_or("") != "preserved")
    {
        throw std::runtime_error("Raw document IO discarded an unknown field");
    }
    auto parsedScene = db::loadScene(inputPath);
    auto node = parsedScene->rootNode()->children().at(0);
    auto drawable = node->getComponent<scene::DrawableComponent>();
    auto environment = node->getComponent<scene::EnvironmentComponent>();
    if (!drawable || !environment || drawable->executionMode() != scene::DrawableComponent::ExecutionMode::Uninitialized ||
        drawable->priority() != 7 || drawable->resourcePath() != missingMesh || node->enabled() || !node->steady() ||
        std::filesystem::absolute(environment->environmentImage()).lexically_normal() != missingEnvironment)
    {
        throw std::runtime_error("CPU graph/reference decoding failed");
    }
    auto cloned = std::dynamic_pointer_cast<scene::DrawableComponent>(drawable->clone());
    if (!cloned || cloned->resourcePath() != missingMesh || cloned->drawDrawable())
    {
        throw std::runtime_error("Unresolved reference clone changed its state");
    }
    // Saving to another directory rebases references without creating resources.
    const auto movedDirectory = directory / "reference-output";
    std::filesystem::create_directories(movedDirectory);
    const auto outputPath = movedDirectory / "references-output.bg2e";
    db::saveSceneReferences(node.get(), outputPath);
    auto roundTrip = db::loadScene(outputPath);
    auto roundTripNode = roundTrip->rootNode()->children().at(0);
    if (roundTripNode->getComponent<scene::DrawableComponent>()->resourcePath() != missingMesh ||
        std::filesystem::absolute(roundTripNode->getComponent<scene::EnvironmentComponent>()->environmentImage()).lexically_normal() != missingEnvironment ||
        std::filesystem::exists(missingMesh) || std::filesystem::exists(missingEnvironment))
    {
        throw std::runtime_error("Reference-only round trip touched or relocated assets");
    }
    auto unknown = JSON(JsonObject{{"type", JSON("UnregisteredDbProbe")}, {"source", JSON("missing.custom")}});
    nodeData->objectValue()["components"]->listValue().push_back(unknown);
    db::writeSceneDocument(document, inputPath);
    read = db::readSceneDocument(inputPath);
    auto nodes = ObjectReader(read).getArray("scene");
    auto components = nodes->getObject(0)->getArray("components");
    if (components->getObject(components->size() - 1)->getString("source").value_or("") != "missing.custom")
    {
        throw std::runtime_error("Raw document IO lost an unknown component");
    }
    bool rejected = false;
    try
    {
        db::loadScene(inputPath);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    if (!rejected)
    {
        throw std::runtime_error("Typed graph load silently dropped an unsupported component");
    }
}

// Supply a real scene referencing BG2/OBJ resources, an initialized Engine, and
// stopped frame/background producers. No production Engine is needed.
inline void runDrawSceneLoadProbe(Engine& engine, const std::filesystem::path& filePath)
{
    auto loaded = db::loadScene(filePath, engine);
    scene::FindNodeComponentVisitor<scene::DrawableComponent> visitor;
    const auto nodes = visitor.find(loaded->rootNode());
    if (nodes.empty())
    {
        throw std::invalid_argument("Draw load probe needs at least one drawable");
    }
    for (const auto& weakNode : nodes)
    {
        auto node = weakNode.lock();
        auto component = node->getComponent<scene::DrawableComponent>();
        auto drawable = component->drawDrawable();
        if (!drawable || !drawable->isLoaded() || drawable->engine() != &engine || !drawable->drawMesh())
        {
            throw std::runtime_error("Scene resource was not realized on the draw engine");
        }
        loaded->rootNode()->loadResources(engine);
        if (component->drawDrawable() != drawable)
        {
            throw std::runtime_error("Repeated same-context realization replaced live resources");
        }
        component->serializeReferences(filePath.parent_path());
    }
    // Release all graph/resource owners before engine cleanup.
}
}
