
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Drawable.hpp>

namespace bg2e::manipulation {

SelectableComponent::SelectableComponent()
{
}

void SelectableComponent::update(float)
{
    if (!ownerNode())
    {
        return;
    }
    
    auto drwComponent = ownerNode()->drawable();
    auto drw = drwComponent ? dynamic_cast<scene::Drawable*>(drwComponent->drawable().get()) : nullptr;
    if (_submeshCount == 0 && drw)
    {
        _submeshCount = drw->submeshesCount();
        for (auto i = 0; i < _submeshCount; ++i)
        {
            _identifier[i] = generateIdentifier();
        }
    }
}

uint32_t SelectableComponent::_lastIdentifier = 0;

uint32_t SelectableComponent::generateIdentifier()
{
    SelectableComponent::_lastIdentifier++;
    return SelectableComponent::_lastIdentifier;
}


scene::BG2E_SCENE_REGISTER_COMPONENT(SelectableComponent);

}
