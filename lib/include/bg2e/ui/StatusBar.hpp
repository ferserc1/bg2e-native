
#pragma once

#include <bg2e/ui/Window.hpp>

#include <vector>
#include <string>

namespace bg2e {
namespace ui {

class BG2E_API StatusItem {
public:
    inline void setText(const std::string & text) { _text = text; }
    inline void setText(std::string&& text) { _text = std::move(text); }
    inline const std::string & getText() const { return _text; }

protected:
    std::string _text;
};

class BG2E_API StatusBar : public Window {
public:

    enum Alignment {
        AlignLeft,
        AlignRight
    };

    void draw() override;

    inline void addItem(std::shared_ptr<StatusItem> item, Alignment align = AlignLeft)
    {
        align == AlignLeft
            ? _leftItems.push_back(item)
            : _rightItems.push_back(item);
    }

protected:
    std::vector<std::shared_ptr<StatusItem>> _leftItems;
    std::vector<std::shared_ptr<StatusItem>> _rightItems;

    void setDrawFunction(std::function<void()> drawFunction) override;
    void draw(std::function<void()> drawFunc, std::function<void()> menuFunction = nullptr) override;

};

}
}