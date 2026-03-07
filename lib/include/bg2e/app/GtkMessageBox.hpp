
#pragma once

#include <bg2e/app/MessageBox.hpp>

#include "MessageBox.hpp"

namespace bg2e::app::internal
{

class GtkMessageBox
{
public:
    GtkMessageBox() = default;

    int32_t showMessage(const std::string& title, const std::string& message);

    void setButtons(const std::vector<MessageBox::Button>& buttons)
    {
        _buttons = buttons;
    }

    inline void setType(const MessageBox::MessageType type) { _messageType = type; }

protected:
    MessageBox::MessageType _messageType = MessageBox::MessageType::Info;
    std::vector<MessageBox::Button> _buttons;
};

}
