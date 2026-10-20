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

#include <bg2e/db/scene.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>

#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class CustomTransform final : public bg2e::scene::TransformComponent {
  public:
    BG2E_COMPONENT_TYPE_NAME("ComposerSchemaCustomTransform");

    std::shared_ptr<bg2e::scene::Component> clone() const override
    {
        auto result = std::make_shared<CustomTransform>(*this);
        result->_owner = nullptr;
        return result;
    }

    void deserializeWithProgress(std::shared_ptr<bg2e::json::JsonNode>, const std::filesystem::path&,
        bg2e::render::Engine&, bg2e::scene::SceneLoadProgress*) override
    {
        throw std::logic_error("Schema IO must never call a production creator");
    }
};

class SlicedTransform final : public bg2e::scene::TransformComponent {
  public:
    BG2E_COMPONENT_TYPE_NAME("ComposerInvalidSlicedTransform");
};

void environmentSelection(const std::filesystem::path& directory)
{
    using namespace bg2e;
    auto root = std::make_shared<scene::Node>("environment scene");
    auto first = std::make_shared<scene::Node>("first environment");
    auto active = std::make_shared<scene::EnvironmentComponent>();
    first->addComponent(active);
    root->addChild(first);
    scene::Scene owner;
    owner.setSceneRoot(root);
    owner.setMainEnvironment(active.get());
    auto ignored = std::make_shared<scene::EnvironmentComponent>();
    root->addComponent(ignored); // Earlier in traversal, but not explicitly selected.
    owner.updateEnvironment();
    require(owner.mainEnvironment() == active.get(), "A newly added environment stole active selection");
    active->setSkyHorizonColor(base::Color(0.9f,0.1f,0.2f));
    active->setEnvironmentImage("sample.hdr");
    active->setEnvironmentImage("");
    require(owner.mainEnvironment() == active.get() && active->skyHorizonColor().r == 0.9f,
        "In-place colors/image clearing lost active environment state");
    first->removeComponent(active);
    owner.updateEnvironment();
    require(owner.hasExplicitMainEnvironment() && !owner.mainEnvironment(),
        "Removing active Environment promoted an ignored component instead of default sky");
    first->addComponent(active);
    owner.setMainEnvironment(active.get());
    first->setEnabled(false);
    require(!owner.mainEnvironment(), "Disabled environment branch remained active");
    owner.setMainEnvironment(nullptr);
    first->setEnabled(true);
    require(!owner.mainEnvironment(), "Explicit default sky was replaced by traversal fallback");
    owner.setMainEnvironment(active.get());
    root->removeChild(first);
    require(!owner.mainEnvironment(), "Detached environment branch remained active");
    bool rejected = false;
    try
    {
        owner.setMainEnvironment(active.get());
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "Environment from outside the scene was accepted");
    auto next = std::make_shared<scene::EnvironmentComponent>();
    first->addComponent(next);
    root->addChild(first);
    owner.setMainEnvironment(next.get());
    require(owner.mainEnvironment() == next.get(), "A new environment could not replace explicit default sky");
    next->setEnvironmentImage("selected.hdr");
    next->setSkyZenithColor(base::Color(0.7f,0.2f,0.1f));
    require(owner.mainEnvironment() == next.get() && next->environmentImage() == "selected.hdr" &&
        std::abs(next->skyZenithColor().r - 0.7f) < 0.0001f,
        "Selected HDR/colors do not remain on the active scene component");
    owner.releaseSceneRoot();
    require(!owner.hasExplicitMainEnvironment() && !owner.mainEnvironment(),
        "Released scene retained active environment references");
    scene::Scene legacy;
    legacy.setSceneRoot(root);
    require(!legacy.hasExplicitMainEnvironment() && legacy.mainEnvironment() == ignored.get(),
        "Explicit selection changed default scene traversal for existing clients");
    legacy.releaseSceneRoot();
    auto disabledRoot = std::make_shared<scene::Node>("disabled environment branch");
    auto disabledNode = std::make_shared<scene::Node>("disabled HDR");
    auto disabledEnvironment = std::make_shared<scene::EnvironmentComponent>();
    disabledNode->addComponent(disabledEnvironment);
    disabledNode->setEnabled(false);
    auto enabledNode = std::make_shared<scene::Node>("enabled HDR");
    auto enabledEnvironment = std::make_shared<scene::EnvironmentComponent>();
    enabledNode->addComponent(enabledEnvironment);
    disabledRoot->addChild(disabledNode);
    disabledRoot->addChild(enabledNode);
    scene::Scene disabledScene;
    disabledScene.setSceneRoot(disabledRoot);
    require(disabledScene.mainEnvironment() == enabledEnvironment.get(),
        "Automatic environment selection chose a disabled branch");
    disabledScene.releaseSceneRoot();
    scene::Scene unattached;
    require(!unattached.hasExplicitMainEnvironment(), "Unattached scene should retain implicit default sky");
    unattached.setMainEnvironment(nullptr);
    require(!unattached.hasExplicitMainEnvironment(), "Renderer startup selection froze defaults before scene load");
    unattached.setSceneRoot(root);
    require(unattached.mainEnvironment() == ignored.get(), "Loaded scene Environment was not selected after unattached startup");
    unattached.releaseSceneRoot();
    auto survivingRoot = std::make_shared<scene::Node>("root retained after scene destruction");
    auto replacementRoot = std::make_shared<scene::Node>("replacement scene root");
    {
        scene::Scene temporary;
        temporary.setSceneRoot(survivingRoot);
        temporary.setSceneRoot(replacementRoot);
        require(!survivingRoot->scene(), "Replaced root retained its previous Scene back-pointer");
    }
    require(!replacementRoot->scene(), "Destroyed Scene left a dangling root back-pointer");

    scene::EnvironmentComponent schema;
    schema.deserialize(json::JSON(json::JsonObject{{"environmentImage",json::JSON("new.hdr")}}), directory);
    require(schema.environmentImage() == std::filesystem::absolute(directory / "new.hdr").lexically_normal().string(),
        "Environment image property alias was not loaded");
    schema.deserialize(json::JSON(json::JsonObject{{"equirectangularTexture",json::JSON("")}}), directory);
    require(schema.environmentImage().empty(), "Empty legacy HDR reference did not restore procedural sky");
    schema.setEnvironmentImage("previous.hdr");
    schema.deserialize(json::JSON(json::JsonObject{}), directory);
    require(schema.environmentImage().empty(), "Missing HDR reference retained a previous image");
}

void run(const std::filesystem::path& directory)
{
    using namespace bg2e;
    environmentSelection(directory);
    scene::RegisterComponent<CustomTransform> registration;
    auto root = std::make_shared<scene::Node>("Root with metadata");
    root->setSteady(true);
    auto custom = std::make_shared<CustomTransform>();
    custom->setTranslation(3, 4, 5);
    custom->setPriority(42);
    root->addComponent(custom);
    require(root->transform() == custom.get(), "Base component lookup lost a user subclass");
    auto cameraNode = std::make_shared<scene::Node>("Camera");
    auto camera = std::make_shared<scene::CameraComponent>();
    auto projection = std::make_shared<math::PerspectiveProjection>();
    projection->setFov(73);
    projection->setNear(0.37f);
    projection->setFar(789);
    camera->setProjection(projection);
    cameraNode->addComponent(camera);
    cameraNode->addComponent(new scene::OrbitCameraComponent());
    root->addChild(cameraNode);
    auto copy = root->clone();
    require(copy->identifier() != root->identifier() && copy->steady(), "Node clone identity/flags lost");
    require(copy->transform() != custom.get() && copy->transform()->priority() == 42,
        "Custom clone is shared or lost common metadata");
    copy->transform()->setTranslation(9, 8, 7);
    require(custom->translation() == glm::vec3(3, 4, 5), "Custom transform clone shares state");
    auto copiedCamera = copy->children().at(0)->camera();
    require(copiedCamera->projection() != projection.get(), "Camera clone shares its projection");
    copiedCamera->projection<math::PerspectiveProjection>()->setFov(41);
    require(projection->fov() == 73, "Changing a camera clone changed the original");
    auto optical = std::make_shared<math::OpticalProjection>();
    optical->setFrameSize(50);
    optical->setFocalLength(55);
    camera->setProjection(optical);
    auto opticalCopy = std::dynamic_pointer_cast<scene::CameraComponent>(camera->clone());
    opticalCopy->projection<math::OpticalProjection>()->setFocalLength(99);
    require(optical->focalLength() == 55, "Optical camera clone shares state");
    camera->setProjection(projection);

    auto invalid = std::make_shared<scene::Node>("Bad clone");
    auto sliced = std::make_shared<SlicedTransform>();
    invalid->addComponent(sliced);
    bool rejected = false;
    try
    {
        (void)invalid->clone();
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected && sliced->ownerNode() == invalid.get(), "Sliced clone was accepted or changed its source");

    const auto path = directory / "composer-schema.json";
    // No engine, asset reads, device or GPU resources are needed for these hooks.
    for (unsigned cycle = 0; cycle < 3; ++cycle)
    {
        db::saveSceneReferences(root.get(), path);
        auto scene = db::loadScene(path);
        auto wrapper = scene->releaseSceneRoot();
        require(!scene->rootNode() && !wrapper->scene(), "Scene root ownership was not detached");
        require(wrapper->children().size() == 1, "A save introduced another top-level root");
        auto next = wrapper->children().front();
        wrapper->removeChild(next);
        require(!next->parent() && !next->scene(), "Editable root is still attached");
        require(next->name() == root->name() && next->steady() && next->children().size() == 1,
            "Repeated IO lost root metadata or nested a synthetic container");
        require(dynamic_cast<CustomTransform*>(next->transform()) && next->transform()->priority() == 42,
            "Schema factory lost the registered custom component");
        auto loadedProjection = next->children().front()->camera()->projection<math::PerspectiveProjection>();
        require(loadedProjection && loadedProjection->fov() == 73 && loadedProjection->near() == 0.37f &&
            loadedProjection->far() == 789, "Perspective camera parameters did not round-trip");
        root = std::move(next);
    }

    // Multi-root transfer retains the synthetic container rather than its raw Scene back-pointer.
    auto scene = std::make_shared<scene::Scene>();
    auto wrapper = std::make_shared<scene::Node>("Multiple roots");
    wrapper->addChild(root);
    wrapper->addChild(new scene::Node("Second root"));
    scene->setSceneRoot(wrapper);
    auto detached = scene->releaseSceneRoot();
    scene.reset();
    require(detached->children().size() == 2 && !detached->scene(), "Multi-root transfer lost content or kept a dangling Scene");
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2 || !std::filesystem::is_directory(argv[1]))
        {
            throw std::invalid_argument("Usage: composer_schema_probe EXISTING_DISPOSABLE_DIRECTORY");
        }
        run(argv[1]);
        std::cout << "Composer schema and clone checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
