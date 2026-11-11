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

#include <bg2e/scene/all.hpp>
#include <stdexcept>
#include <iostream>

namespace {
using namespace bg2e;
using namespace bg2e::scene;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

ComponentIssue issue(const Node& node, const std::string& code)
{
    for (const auto& entry : validateNodeComponents(node))
    {
        if (entry.issue.code == code)
        {
            return entry.issue;
        }
    }
    throw std::runtime_error("Expected issue was not reported: " + code);
}

void sunRepair(bool existingTransform)
{
    auto node = std::make_shared<Node>();
    node->addComponent(std::make_shared<EnvironmentComponent>());
    std::shared_ptr<TransformComponent> original;
    if (existingTransform)
    {
        original = std::make_shared<TransformComponent>();
        original->setPriority(25);
        node->addComponent(original);
    }
    const auto before = node->orderedComponents();
    auto command = issue(*node, "environment.source.missing").fixes.front().createCommand();
    require(node->orderedComponents() == before, "Factory must not mutate the node");
    require(command->execute().status == app::CommandStatus::Applied, "Sun repair failed");
    auto* sun = node->getComponent<LightComponent>();
    auto* transform = node->getComponent<TransformComponent>();
    require(sun && sun->light().type() == base::Light::TypeSun && transform, "Incomplete Sun repair");
    require(!original || transform == original.get(), "Existing transformation was replaced");
    require(command->undo().status == app::CommandStatus::Applied, "Sun undo failed");
    require(node->orderedComponents() == before, "Undo changed preexisting membership or order");
    require(command->redo().status == app::CommandStatus::Applied, "Sun redo failed");
    require(node->getComponent<LightComponent>() == sun, "Redo created another light identity");
    require(node->getComponent<TransformComponent>() == transform, "Redo changed transform identity");
}

void existingLight()
{
    auto node = std::make_shared<Node>();
    node->addComponent(std::make_shared<EnvironmentComponent>());
    auto light = std::make_shared<LightComponent>();
    light->light().setType(base::Light::TypeSpot);
    light->light().setIntensity(7.0f);
    node->addComponent(light);
    auto command = issue(*node, "environment.source.missing").fixes.front().createCommand();
    require(command->execute().status == app::CommandStatus::Applied, "Light conversion failed");
    require(light->light().type() == base::Light::TypeSun, "Light was not converted to Sun");
    require(command->undo().status == app::CommandStatus::Applied, "Light conversion undo failed");
    require(node->getComponent<LightComponent>() == light.get(), "Undo removed an existing light");
    require(light->light().type() == base::Light::TypeSpot && light->light().intensity() == 7.0f,
        "Undo did not restore the original light configuration");
    require(!node->getComponent<TransformComponent>(), "Undo retained its new transformation");
}

void independentDependencies()
{
    auto node = std::make_shared<Node>();
    node->addComponent(std::make_shared<OrbitCameraComponent>());
    require(issue(*node, "transform.missing").severity == ComponentIssueSeverity::Error, "Transform must be an error");
    require(issue(*node, "orbit.camera.missing").severity == ComponentIssueSeverity::Warning, "Camera must be a warning");
    auto camera = issue(*node, "orbit.camera.missing").fixes.front().createCommand();
    require(camera->execute().status == app::CommandStatus::Applied, "Camera addition failed");
    require(node->getComponent<CameraComponent>()->projection() != nullptr, "Suggested camera needs a projection");
    require(node->getComponent<TransformComponent>() != nullptr, "Suggested camera requires a transformation");
    require(camera->undo().status == app::CommandStatus::Applied, "Camera undo failed");
    require(!node->getComponent<CameraComponent>() && !node->getComponent<TransformComponent>(),
        "Camera undo must remove both components created by the fix");
}

void cameraWithExistingTransform()
{
    auto node = std::make_shared<Node>();
    node->addComponent(std::make_shared<OrbitCameraComponent>());
    auto transform = std::make_shared<TransformComponent>();
    node->addComponent(transform);
    auto command = issue(*node, "orbit.camera.missing").fixes.front().createCommand();
    require(command->execute().status == app::CommandStatus::Applied, "Camera repair failed");
    require(command->undo().status == app::CommandStatus::Applied, "Camera repair undo failed");
    require(node->getComponent<TransformComponent>() == transform.get(), "Camera undo removed the existing Transform");
    require(!node->getComponent<CameraComponent>(), "Camera undo retained its added Camera");
}

void staleFixes()
{
    auto node = std::make_shared<Node>();
    node->addComponent(std::make_shared<CameraComponent>());
    auto stale = issue(*node, "transform.missing").fixes.front().createCommand();
    auto transform = std::make_shared<TransformComponent>();
    node->addComponent(transform);
    require(stale->execute().status == app::CommandStatus::NoOp, "Stale fix must not duplicate a transformation");
    require(node->getComponent<TransformComponent>() == transform.get(), "Stale fix replaced the transformation");

    auto other = std::make_shared<Node>();
    auto camera = std::make_shared<CameraComponent>();
    other->addComponent(camera);
    auto expired = issue(*other, "transform.missing").fixes.front().createCommand();
    other->removeComponent(camera);
    require(expired->execute().status == app::CommandStatus::Rejected, "Detached source must be rejected");
    require(!other->getComponent<TransformComponent>(), "Rejected fix mutated the node");
}

class EditorOverlay final : public DrawableComponent {
  public:
    BG2E_COMPONENT_TYPE_NAME("TestEditorOverlay");

    bool isSceneGeometry() const override
    {
        return false;
    }
};

void textWithEditorOverlay()
{
    auto node = std::make_shared<Node>();
    auto text = std::make_shared<TextComponent>();
    node->addComponent(text);
    require(!text->checkCompatibility(*text), "Text must not conflict with itself");
    node->addComponent(std::make_shared<EditorOverlay>());
    for (const auto& entry : validateNodeComponents(*node))
    {
        require(entry.issue.code != "text.drawable.overlap", "Editor overlays must not conflict with Text");
    }
}

void directionalCompatibility()
{
    auto node = std::make_shared<Node>();
    auto text = std::make_shared<TextComponent>();
    auto drawable = std::make_shared<DrawableComponent>();
    node->addComponent(text);
    node->addComponent(drawable);
    require(text->checkCompatibility(*drawable).has_value(), "Text must declare the conflict");
    require(!drawable->checkCompatibility(*text).has_value(), "Drawable must not know its derived types");
    size_t count = 0;
    for (const auto& entry : validateNodeComponents(*node))
    {
        if (entry.issue.code == "text.drawable.overlap")
        {
            ++count;
            require(entry.component.lock() == text && entry.relatedComponent.lock() == drawable,
                "Conflict attributed to the wrong component");
        }
    }
    require(count == 1, "Directional conflict was duplicated");
}

void parentDependency()
{
    auto parent = std::make_shared<Node>();
    auto child = std::make_shared<Node>();
    child->addComponent(std::make_shared<InputChainJointComponent>());
    parent->addChild(child);
    require(issue(*child, "chain.parent.missing").fixes.empty(), "Parent edits require explicit help only");
    parent->addComponent(std::make_shared<ChainComponent>());
    require(validateNodeComponents(*child).empty(), "Immediate parent must satisfy the chain dependency");
}

void fontDiagnostics()
{
    Scene scene;
    auto root = std::make_shared<Node>();
    auto textNode = std::make_shared<Node>();
    root->addChild(textNode);
    scene.setSceneRoot(root);
    textNode->addComponent(std::make_shared<TextComponent>());
    require(issue(*textNode, "text.fonts.missing").severity == ComponentIssueSeverity::Error,
        "Missing font collection must be an error");
    root->addComponent(std::make_shared<TextFontComponent>());
    require(issue(*textNode, "text.font.unconfigured").severity == ComponentIssueSeverity::Error,
        "Empty font collection must have its own error");
}

}

int main()
{
    try
    {
        sunRepair(false);
        sunRepair(true);
        existingLight();
        independentDependencies();
        cameraWithExistingTransform();
        staleFixes();
        textWithEditorOverlay();
        directionalCompatibility();
        parentDependency();
        fontDiagnostics();
        std::cout << "Component issue probes passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
