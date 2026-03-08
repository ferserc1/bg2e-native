//
//  Toolbar.cpp

#include <bg2e/ui/Toolbar.hpp>
#include <bg2e/ui/BasicWidgets.hpp>

#include <iostream>
#include <algorithm>

namespace bg2e::ui {

int32_t Toolbar::addButton(
    ToolbarButton & button,
    Alignment align
) {
    auto & vec = align == AlignLeft ? _leftButtons : _rightButtons;
    if (button.id < 0)
    {
        button.id = genId();
    }
    vec.push_back(button);
    return button.id;
}

int32_t Toolbar::addButton(
    ToolbarButton && button,
    Alignment align
) {
    auto & vec = align == AlignLeft ? _leftButtons : _rightButtons;
    if (button.id < 0)
    {
        button.id = genId();
    }
    vec.push_back(std::move(button));
    return button.id;
}

void Toolbar::draw()
{
    draw([&]() {
        for (auto & btn : _leftButtons)
        {
            if (btn.action != nullptr && BasicWidgets::button(btn.label, true, btn.disabled))
            {
                btn.action();
            }
            else if (btn.action == nullptr) {
                BasicWidgets::text(btn.label, true);
            }
        }
        
        auto rightButtonsSize = 0;
        for (auto & btn : _rightButtons)
        {
            if (btn.action != nullptr)
            {
                rightButtonsSize += BasicWidgets::calcButtonWidth(btn.label) +
                    BasicWidgets::getItemHorizontalSpacing();
            }
            else
            {
                rightButtonsSize += BasicWidgets::calcTextWidth(btn.label) +
                    BasicWidgets::getItemHorizontalSpacing();
            }
        }
        auto btnIndex = 0;
        for (auto & btn : _rightButtons)
        {
            btnIndex == 0 ? BasicWidgets::sameLine(-rightButtonsSize) : BasicWidgets::sameLine();
            if (btn.action != nullptr && BasicWidgets::button(btn.label, false, btn.disabled))
            {
                btn.action();
            }
            else if (btn.action == nullptr)
            {
                BasicWidgets::text(btn.label);
            }
            ++btnIndex;
        }
    });
}

void Toolbar::updateButtonLabel(int32_t buttonId, const std::string & newLabel)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->label = newLabel;
    }
}

void Toolbar::updateButtonLabel(int32_t buttonId, std::string && newLabel)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->label = newLabel;
    }
}

void Toolbar::enableButton(int32_t buttonId)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->disabled = false;
    }
}

void Toolbar::disableButton(int32_t buttonId)
{
    auto btn = findButton(buttonId);
    if (btn)
    {
        btn->disabled = true;
    }
}

ToolbarButton * Toolbar::findButton(int32_t buttonId)
{
    auto result = std::find_if(_leftButtons.begin(), _leftButtons.end(), [buttonId](const ToolbarButton& btn) -> bool {
        return btn.id == buttonId;
    });
    if (result == _leftButtons.end())
    {
        result = std::find_if(_rightButtons.begin(), _rightButtons.end(), [buttonId](const ToolbarButton& btn) -> bool {
            return btn.id == buttonId;
        });
    }
    if (result == _rightButtons.end())
    {
        return nullptr;
    }
    return &(*result);
}

void Toolbar::setDrawFunction(std::function<void()> /* drawFunction */)
{
    // Not used
}

void Toolbar::draw(
    std::function<void()> drawFunc,
    std::function<void()> menuFunction
) {
    Window::draw(drawFunc, menuFunction);
    if (!_menuInitialized)
    {
        setMenuFunction([&]() {
            _menu.draw();
        });
        _menuInitialized = false;
    }
}

int32_t Toolbar::genId()
{
    // Generate a unique button id not used by any button in _leftButtons or _rightButtons
    auto idInUse = [this](int32_t candidate) -> bool {
        for (const auto & b : _leftButtons) {
            if (b.id == candidate) return true;
        }
        for (const auto & b : _rightButtons) {
            if (b.id == candidate) return true;
        }
        return false;
    };

    int32_t candidate = 0;
    while (idInUse(candidate)) {
        ++candidate;
    }
    return candidate;
}

}
