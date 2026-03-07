
#include <bg2e/app/Shortcuts.hpp>

#include <iostream>

namespace bg2e::app
{

void Shortcuts::keyDown(const KeyEvent& evt) const
{
    if (_keyMap.contains(evt.key()))
    {
        const auto fn = _keyMap.find(evt.key())->second;
        fn(evt);
    }
}

void Shortcuts::keyUp(const KeyEvent& evt) const
{
    std::cout << KeyEvent::keyName(evt.key()) << " up" << std::endl;
}

void Shortcuts::addShortcutMapper(const KeyEvent::Key key, const ShortcutHandler& handler)
{
    _keyMap[key] = handler;
}

}
