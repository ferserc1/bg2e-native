//

#include <bg2e/ui/StatusBar.hpp>
#include <bg2e/ui/BasicWidgets.hpp>

namespace bg2e::ui {

void StatusBar::draw()
{
    draw([&]()
    {
        auto textSize = BasicWidgets::calcTextHeight("text") + BasicWidgets::getItemVerticalSpacing();
        BasicWidgets::padding(0, height() / 2 - textSize);

        auto sameLine = false;
        for (auto & item : _leftItems)
        {
            BasicWidgets::text(item->getText(), sameLine);
            sameLine = true;
        }

        auto rightButtonsSize = 0;
        for (auto & item : _rightItems)
        {
            rightButtonsSize += BasicWidgets::calcTextWidth(item->getText()) +
                BasicWidgets::getItemHorizontalSpacing();
        }

        auto txtIndex = 0;
        for (auto & item : _rightItems)
        {
            txtIndex == 0 ? BasicWidgets::sameLine(-rightButtonsSize) : BasicWidgets::sameLine();
            BasicWidgets::text(item->getText(), false);
            ++txtIndex;
        }
    });
}

void StatusBar::setDrawFunction([[maybe_unused]] std::function<void()> drawFunction)
{
}

void StatusBar::draw(
    std::function<void()> drawFunc,
    std::function<void()> menuFunction
) {
    Window::draw(drawFunc, menuFunction);
}

}