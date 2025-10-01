//
//  Toolbar.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/ui/Window.hpp>

#include <string>
#include <functional>
#include <vector>
#include <utility>

namespace bg2e {
namespace ui {

struct ToolbarButton {
    int32_t id = -1;
    std::string label;
    std::function<void()> action = nullptr;
    bool disabled = false;
};

class BG2E_API Toolbar : public Window {
public:
    enum Alignment {
        AlignLeft,
        AlignRight
    };
    
    /**
     * Adds a button to the toolbar.
     * - If button.id < 0, a unique ID will be generated for this Toolbar.
     * - The button will be inserted on the side specified by 'align'.
     * - The ID will be unique among all buttons in this Toolbar (left and right).
     *
     * @param button Button to add (reference). The id attribute in button can be changed if button.id < 0
     * @param align Button alignment (AlignLeft or AlignRight). Defaults to AlignLeft.
     * @return The button ID in this Toolbar. If a new one was generated, the generated ID is returned.
     */
    int32_t addButton(
        ToolbarButton & button,
        Alignment align = AlignLeft
    );
    
    /**
     * Move-enabled overload of 'addButton'.
     * - If button.id < 0, a unique ID will be generated for this Toolbar.
     * - The button will be inserted on the side specified by 'align'.
     * - The ID will be unique among all buttons in this Toolbar (left and right).
     *
     * @param button Button to add (will be moved into the Toolbar).
     * @param align Button alignment (AlignLeft or AlignRight). Defaults to AlignLeft.
     * @return The button ID in this Toolbar. If a new one was generated, the generated ID is returned.
     */
    int32_t addButton(
        ToolbarButton && button,
        Alignment align = AlignLeft
    );

    void draw() override;
    
    void updateButtonLabel(int32_t buttonId, const std::string & newLabel);
    void updateButtonLabel(int32_t buttonId, std::string && newLabel);
    void enableButton(int32_t buttonId);
    void disableButton(int32_t buttonId);
    
    ToolbarButton * findButton(int32_t buttonId);

protected:
    std::vector<ToolbarButton> _leftButtons;
    std::vector<ToolbarButton> _rightButtons;
    
    void setDrawFunction(std::function<void()> drawFunction) override;
    void draw(std::function<void()> drawFunc, std::function<void()> menuFunction = nullptr) override;
    
    int32_t genId();
};

}
}
