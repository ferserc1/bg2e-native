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

#pragma once

#include <bg2e/app/MessageBox.hpp>

#include <cstdint>

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
