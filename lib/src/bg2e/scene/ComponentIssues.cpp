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

#include <bg2e/scene/ComponentIssues.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/scene/TextFontComponent.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/Chain.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace bg2e::scene {
namespace {

template <class T>
const T* find(const Node& node)
{
    for (const auto& component : node.orderedComponents())
    {
        if (auto* result = dynamic_cast<const T*>(component.get()))
        {
            return result;
        }
    }
    return nullptr;
}

void findFonts(const Node& node, std::vector<const TextFontComponent*>& fonts)
{
    if (auto* font = find<TextFontComponent>(node))
    {
        fonts.push_back(font);
    }
    for (const auto& child : node.children())
    {
        findFonts(*child, fonts);
    }
}

enum class FixKind
{
    Transform,
    Camera,
    Environment,
    Sun,
    Font
};

// These fixes retain only their own new components. Existing component identity
// is weakly captured, so history cannot silently retain unaccounted GPU assets.
class FixCommand final : public app::Command {
  public:
    FixCommand(std::weak_ptr<Node> node, std::weak_ptr<Component> source,
        FixKind kind, std::string fontIdentifier, std::weak_ptr<Node> root)
        : _node(std::move(node)), _source(std::move(source)), _kind(kind),
          _fontIdentifier(std::move(fontIdentifier)), _root(std::move(root))
    {
    }

    const app::CommandInfo& info() const noexcept override
    {
        return _info;
    }

    app::CommandMemory memory() const override
    {
        // Admission reserves preparation, both snapshots and rollback copies.
        if (_memoryBytes)
        {
            return {_memoryBytes, {}};
        }
        size_t bytes = sizeof(*this) + 65536;
        const auto reserve = [&bytes](size_t count, size_t unit)
        {
            if (count > (std::numeric_limits<size_t>::max() - bytes) / unit)
            {
                throw std::length_error("Issue fix memory overflow");
            }
            bytes += count * unit;
        };
        reserve(_fontIdentifier.capacity(), 8);
        if (_kind == FixKind::Font)
        {
            reserve(base::PlatformTools::assetPath().native().size(), 64 * sizeof(std::filesystem::path::value_type));
        }
        if (auto node = _node.lock())
        {
            reserve(node->orderedComponents().size(), 16 * (sizeof(std::weak_ptr<Component>) + sizeof(std::shared_ptr<Component>) + sizeof(std::string) + 32));
            if (auto* fonts = find<TextFontComponent>(*node))
            {
                for (const auto& declaration : fonts->declarations())
                {
                    reserve(sizeof(declaration), 16);
                    reserve(declaration.identifier.capacity(), 16);
                    reserve(declaration.path.native().size(), 32 * sizeof(std::filesystem::path::value_type));
                    for (const auto& notice : declaration.notices)
                    {
                        reserve(sizeof(notice), 16);
                        reserve(notice.native().size(), 32 * sizeof(std::filesystem::path::value_type));
                    }
                }
            }
        }
        return {bytes, {}};
    }

    app::CommandResult execute() noexcept override
    {
        using namespace app;
        auto node = _node.lock();
        auto source = _source.lock();
        if (_captured || !node || !source || !source->ownerNode() ||
            source->ownerNode()->sceneRoot() != node->sceneRoot() ||
            (_kind != FixKind::Font && source->ownerNode() != node.get()))
        {
            return {CommandStatus::Rejected, "Issue target is stale"};
        }
        bool publishing = false;
        try
        {
            _memoryBytes = memory().localBytes;
            _before = node->orderedComponents();
            _after = _before;
            _created.clear();
            _fontBefore.clear();
            _fontAfter.clear();
            _fonts.reset();
            _light.reset();
            switch (_kind)
            {
            case FixKind::Transform:
                addIfMissing<TransformComponent>(*node);
                break;
            case FixKind::Camera:
                if (!find<CameraComponent>(*node))
                {
                    auto camera = std::make_shared<CameraComponent>();
                    camera->setProjection(std::make_shared<math::PerspectiveProjection>());
                    add(camera);
                }
                addIfMissing<TransformComponent>(*node);
                break;
            case FixKind::Environment:
                if (auto* sun = dynamic_cast<LightComponent*>(source.get());
                    !sun || sun->light().type() != base::Light::TypeSun)
                {
                    return {};
                }
                addIfMissing<EnvironmentComponent>(*node);
                break;
            case FixKind::Sun:
                if (auto* environment = dynamic_cast<EnvironmentComponent*>(source.get());
                    !environment || !environment->environmentImage().empty())
                {
                    return {};
                }
                if (auto* light = node->getComponent<LightComponent>())
                {
                    if (light->light().type() != base::Light::TypeSun)
                    {
                        _light = std::dynamic_pointer_cast<LightComponent>(light->shared_from_this());
                        _lightBefore = light->light().type();
                    }
                }
                else
                {
                    auto sun = std::make_shared<LightComponent>();
                    sun->light().setType(base::Light::TypeSun);
                    add(sun);
                }
                addIfMissing<TransformComponent>(*node);
                break;
            case FixKind::Font:
                prepareFont(*node);
                break;
            }
            if (_created.empty() && _light.expired() && _fontBefore == _fontAfter)
            {
                _before.clear();
                _after.clear();
                return {};
            }
            std::stable_sort(_after.begin(), _after.end(), [](const auto& a, const auto& b)
            {
                return a->priority() > b->priority();
            });
            _beforeWeak.assign(_before.begin(), _before.end());
            _afterWeak.assign(_after.begin(), _after.end());
            publishing = true;
            publish(*node, _after, true);
            _captured = true;
            _applied = true;
            _before.clear();
            _after.clear();
            return {CommandStatus::Applied, {}};
        }
        catch (...)
        {
            if (publishing)
            {
                try
                {
                    publish(*node, _before, false);
                }
                catch (...)
                {
                    _before.clear();
                    _after.clear();
                    return {CommandStatus::Faulted, "Issue fix rollback failed"};
                }
            }
            _before.clear();
            _after.clear();
            return {CommandStatus::Failed, "Issue fix could not be applied"};
        }
    }

    app::CommandResult undo() noexcept override
    {
        return restore(false);
    }

    app::CommandResult redo() noexcept override
    {
        return restore(true);
    }

  private:
    std::weak_ptr<Node> _node;
    std::weak_ptr<Component> _source;
    FixKind _kind;
    std::string _fontIdentifier;
    std::weak_ptr<Node> _root;
    app::CommandInfo _info{"Fix scene component", "scene.component.fix"};
    std::vector<std::shared_ptr<Component>> _before, _after, _created;
    std::vector<std::weak_ptr<Component>> _beforeWeak, _afterWeak;
    std::weak_ptr<TextFontComponent> _fonts;
    std::vector<TextFontComponent::Declaration> _fontBefore, _fontAfter;
    std::weak_ptr<LightComponent> _light;
    base::Light::LightType _lightBefore = base::Light::TypeDisabled;
    size_t _memoryBytes = 0;
    bool _captured = false;
    bool _applied = false;

    void add(std::shared_ptr<Component> component)
    {
        _after.push_back(component);
        _created.push_back(std::move(component));
    }

    template <class T>
    void addIfMissing(const Node& node)
    {
        if (!find<T>(node))
        {
            add(std::make_shared<T>());
        }
    }

    void prepareFont(Node& node)
    {
        auto root = _root.lock();
        auto source = std::dynamic_pointer_cast<TextComponent>(_source.lock());
        if (!root || !source || source->text().fontIdentifier() != _fontIdentifier)
        {
            throw std::runtime_error("Text font issue changed");
        }
        bool belongs = false;
        for (auto* ancestor = source->ownerNode(); ancestor; ancestor = ancestor->parent())
        {
            belongs = belongs || ancestor == root.get();
        }
        if (!belongs)
        {
            throw std::runtime_error("Text left its document");
        }
        std::vector<const TextFontComponent*> declarations;
        findFonts(*root, declarations);
        for (auto* collection : declarations)
        {
            for (const auto& declaration : collection->declarations())
            {
                if (declaration.identifier == _fontIdentifier)
                {
                    return;
                }
            }
        }
        auto* existing = node.getComponent<TextFontComponent>();
        auto fonts = existing ? std::dynamic_pointer_cast<TextFontComponent>(existing->shared_from_this()) :
            std::make_shared<TextFontComponent>();
        _fonts = fonts;
        _fontBefore = fonts->declarations();
        _fontAfter = _fontBefore;
        // Never replace an authored declaration that appeared after diagnosis.
        for (const auto& declaration : _fontBefore)
        {
            if (declaration.identifier == _fontIdentifier)
            {
                return;
            }
        }
        if (_fontAfter.size() >= TextFontComponent::MaxDeclarations)
        {
            throw std::runtime_error("Font declaration limit reached");
        }
        auto directory = std::filesystem::absolute(base::PlatformTools::assetPath() /
            "fonts/didact-gothic").lexically_normal();
        TextFontComponent::Declaration declaration{_fontIdentifier,
            directory / "DidactGothic-Regular.ttf", 0,
            {directory / "OFL.txt", directory / "README.md"}};
        if (!TextFontComponent::validateDeclaration(declaration) ||
            !std::filesystem::is_regular_file(declaration.path))
        {
            throw std::runtime_error("Default font is unavailable");
        }
        _fontAfter.push_back(std::move(declaration));
        if (!existing)
        {
            add(fonts);
        }
    }

    void publish(Node& node, const std::vector<std::shared_ptr<Component>>& components, bool forward)
    {
        node.setComponentsOrdered(components);
        if (auto light = _light.lock())
        {
            light->light().setType(forward ? base::Light::TypeSun : _lightBefore);
        }
        if (auto fonts = _fonts.lock())
        {
            const auto& declarations = forward ? _fontAfter : _fontBefore;
            fonts->setDeclarations(declarations);
            if (fonts->declarations() != declarations)
            {
                throw std::runtime_error("Font declarations could not be restored");
            }
        }
        if (auto* scene = node.scene())
        {
            scene->updateAll();
            if (_kind == FixKind::Font)
            {
                scene->invalidateTextFonts();
            }
        }
    }

    static std::vector<std::shared_ptr<Component>> lock(
        const std::vector<std::weak_ptr<Component>>& snapshot)
    {
        std::vector<std::shared_ptr<Component>> result;
        result.reserve(snapshot.size());
        for (const auto& weak : snapshot)
        {
            auto component = weak.lock();
            if (!component)
            {
                throw std::runtime_error("Component snapshot expired");
            }
            result.push_back(std::move(component));
        }
        return result;
    }

    app::CommandResult restore(bool forward) noexcept
    {
        using namespace app;
        auto node = _node.lock();
        if (!_captured || !node || _applied == forward)
        {
            return {CommandStatus::Rejected, "Issue fix replay is unavailable"};
        }
        bool publishing = false;
        std::vector<std::shared_ptr<Component>> previous;
        try
        {
            previous = lock(forward ? _beforeWeak : _afterWeak);
            if (node->orderedComponents() != previous)
            {
                return {CommandStatus::Rejected, "Issue fix membership changed outside history"};
            }
            if (auto fonts = _fonts.lock(); fonts &&
                fonts->declarations() != (forward ? _fontBefore : _fontAfter))
            {
                return {CommandStatus::Rejected, "Font declarations changed outside history"};
            }
            if (auto light = _light.lock(); light &&
                light->light().type() != (forward ? _lightBefore : base::Light::TypeSun))
            {
                return {CommandStatus::Rejected, "Light configuration changed outside history"};
            }
            auto next = lock(forward ? _afterWeak : _beforeWeak);
            publishing = true;
            publish(*node, next, forward);
            _applied = forward;
            return {CommandStatus::Applied, {}};
        }
        catch (...)
        {
            if (publishing)
            {
                try
                {
                    publish(*node, previous, !forward);
                }
                catch (...)
                {
                    return {CommandStatus::Faulted, "Issue fix replay recovery failed"};
                }
            }
            return {CommandStatus::Failed, "Issue fix replay failed"};
        }
    }
};

ComponentIssueFix fix(const Node& node, const Component& source, FixKind kind,
    std::string label, std::string description, std::string fontIdentifier = {}, const Node* root = nullptr)
{
    return {std::move(label), std::move(description),
        [target = const_cast<Node&>(node).weak_from_this(),
         component = const_cast<Component&>(source).weak_from_this(), kind,
         identifier = std::move(fontIdentifier),
         scope = root ? const_cast<Node*>(root)->weak_from_this() : std::weak_ptr<Node>{}]
        {
            return std::make_unique<FixCommand>(target, component, kind, identifier, scope);
        }};
}

void requireTransform(const Component& source, const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues)
{
    if (!find<TransformComponent>(context.node))
    {
        issues.push_back({"transform.missing", ComponentIssueSeverity::Error,
            "This component requires a transformation on this node.",
            "Add a Transform component to define its position, orientation and scale.",
            {fix(context.node, source, FixKind::Transform, "Add Transform", "Add a transformation to this node.")}});
    }
}

}

std::optional<ComponentIssue> Component::checkCompatibility(const Component&) const
{
    return std::nullopt;
}

void Component::collectDependencyIssues(const ComponentValidationContext&, std::vector<ComponentIssue>&) const
{
}

std::vector<ComponentIssueEntry> validateNodeComponents(const Node& node, const Node* root)
{
    std::vector<ComponentIssueEntry> result;
    if (root)
    {
        bool belongs = false;
        for (auto* ancestor = &const_cast<Node&>(node); ancestor; ancestor = ancestor->parent())
        {
            belongs = belongs || ancestor == root;
        }
        if (!belongs)
        {
            throw std::invalid_argument("Component validation root must contain the inspected node");
        }
    }
    const auto& components = node.orderedComponents();
    const ComponentValidationContext context{node, const_cast<Node&>(node).scene(), root};
    for (const auto& component : components)
    {
        std::vector<ComponentIssue> dependencies;
        component->collectDependencyIssues(context, dependencies);
        for (auto& issue : dependencies)
        {
            result.push_back({component, {}, std::move(issue)});
        }
        for (const auto& other : components)
        {
            if (component != other)
            {
                if (auto issue = component->checkCompatibility(*other))
                {
                    result.push_back({component, other, std::move(*issue)});
                }
            }
        }
    }
    return result;
}

void CameraComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    requireTransform(*this, context, issues);
}

void OrbitCameraComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    requireTransform(*this, context, issues);
    if (!find<CameraComponent>(context.node))
    {
        issues.push_back({"orbit.camera.missing", ComponentIssueSeverity::Warning,
            "The orbit controller has no camera on this node.",
            "Add a Camera component to use this controller to navigate a camera.",
            {fix(context.node, *this, FixKind::Camera, "Add Camera", "Add a perspective camera and a Transform if missing on this node.")}});
    }
}

void PolarTransformControllerComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    requireTransform(*this, context, issues);
}

void FixedScaleTransformControllerComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    requireTransform(*this, context, issues);
}

void ChainJointComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    auto* parent = const_cast<Node&>(context.node).parent();
    if (!parent || !find<ChainComponent>(*parent))
    {
        issues.push_back({"chain.parent.missing", ComponentIssueSeverity::Warning,
            "The parent node has no Chain component.",
            "Place this joint directly under a node containing a Chain component.", {}});
    }
}

void LightComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    if (light().type() == base::Light::TypeSun && !find<EnvironmentComponent>(context.node))
    {
        issues.push_back({"sun.environment.missing", ComponentIssueSeverity::Warning,
            "This Sun has no associated environment.",
            "Add an Environment component to the same node.",
            {fix(context.node, *this, FixKind::Environment, "Add Environment", "Add an environment to this node.")}});
    }
}

void EnvironmentComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    auto* light = find<LightComponent>(context.node);
    if (environmentImage().empty() && (!light || light->light().type() != base::Light::TypeSun))
    {
        issues.push_back({"environment.source.missing", ComponentIssueSeverity::Warning,
            "This environment has neither a Sun nor a skybox image.",
            "Configure an HDR skybox image or associate a Sun on this node.",
            {fix(context.node, *this, FixKind::Sun, light ? "Use light as Sun" : "Create Sun",
                "Set the light on this node to Sun, or create it, and add a Transform if missing.")}});
    }
}

std::optional<ComponentIssue> TextComponent::checkCompatibility(const Component& other) const
{
    const auto* drawable = dynamic_cast<const DrawableComponent*>(&other);
    if (&other != this && drawable && drawable->isSceneGeometry())
    {
        return ComponentIssue{"text.drawable.overlap", ComponentIssueSeverity::Warning,
            "Text and another Drawable share this node; behavior is undefined.",
            "Move the text or the other drawable to a separate node.", {}};
    }
    return std::nullopt;
}

void TextComponent::collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const
{
    const auto* root = context.root ? context.root : const_cast<Node&>(context.node).sceneRoot();
    std::vector<const TextFontComponent*> fonts;
    if (context.scene && root)
    {
        findFonts(*root, fonts);
    }
    const auto identifier = text().fontIdentifier();
    const TextFontComponent* target = fonts.empty() ? nullptr : fonts.front();
    size_t matches = 0;
    bool configured = false;
    for (auto* font : fonts)
    {
        for (const auto& declaration : font->declarations())
        {
            if (declaration.identifier == identifier)
            {
                ++matches;
                target = font;
                configured = !declaration.path.empty();
            }
        }
    }
    if (matches == 1 && configured && (!context.scene || !context.scene->textFontsReady() ||
        context.scene->textFontAvailable(identifier)))
    {
        return;
    }
    ComponentIssue issue{fonts.empty() ? "text.fonts.missing" : "text.font.unconfigured",
        ComponentIssueSeverity::Error,
        fonts.empty() ? "The scene has no TextFonts component." :
            "The font required by this text is unconfigured, ambiguous or unavailable.",
        "Configure a loadable font '" + identifier + "' in a TextFonts component. Remove duplicate identifiers.", {}};
    if (context.scene && root && matches == 0 && !identifier.empty())
    {
        const auto* destination = target ? target->ownerNode() : root;
        issue.fixes.push_back(fix(*destination, *this, FixKind::Font,
            fonts.empty() ? "Add default font" : "Configure default font",
            "Configure the bundled default font for '" + identifier + "' on node '" + destination->name() + "'.",
            identifier, root));
    }
    issues.push_back(std::move(issue));
}

}
