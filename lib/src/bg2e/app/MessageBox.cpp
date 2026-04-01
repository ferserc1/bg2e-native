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

#include <bg2e/app/MessageBox.hpp>
#include <bg2e/app/GtkMessageBox.hpp>

#include <SDL2/SDL.h>

namespace bg2e::app {

int32_t MessageBox::showInfo(
    const std::string& title,
    const std::string& message,
    const std::vector<Button>& buttons
) {
    return showMessage(Info, title, message, buttons);
}

int32_t MessageBox::showWarning(
    const std::string& title,
    const std::string& message,
    const std::vector<Button>& buttons
) {
    return showMessage(Warning, title, message, buttons);
}

int32_t MessageBox::showError(
    const std::string& title,
    const std::string& message,
    const std::vector<Button>& buttons
) {
    return showMessage(Info, title, message, buttons);
}

int32_t MessageBox::showInfo(
    const std::string& title,
    const std::string& message
) {
    return showMessage(Info, title, message, {});
}

int32_t MessageBox::showWarning(
    const std::string& title,
    const std::string& message
) {
    return showMessage(Warning, title, message, {});
}

int32_t MessageBox::showError(
    const std::string& title,
    const std::string& message
) {
    return showMessage(Error, title, message, {});
}

int32_t MessageBox::showMessage(
    MessageType type,
    const std::string & title,
    const std::string & message,
    const std::vector<Button>& buttons
) {
#ifdef BG2E_LINUX
    internal::GtkMessageBox gtkMessageBox;
    gtkMessageBox.setButtons(buttons);
    gtkMessageBox.setType(type);
    return gtkMessageBox.showMessage(title, message);
#else
    std::vector<SDL_MessageBoxButtonData> sdlButtons;
    int sdlType = type == Info ? SDL_MESSAGEBOX_INFORMATION :
        type == Warning ? SDL_MESSAGEBOX_WARNING : SDL_MESSAGEBOX_ERROR;
    
    for (auto & b : buttons)
    {
        auto key = b.key == ButtonDefaultKey::Esc ? SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT :
            b.key == ButtonDefaultKey::Return ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : 0;
        
        sdlButtons.push_back({
            static_cast<Uint32>(key), static_cast<int>(b.code), b.label.c_str()
        });
    }
    
    const SDL_MessageBoxData messageBoxData = {
        static_cast<Uint32>(sdlType),
        nullptr,
        title.c_str(),
        message.c_str(),
        static_cast<int>(sdlButtons.size()),
        sdlButtons.data(),
        nullptr
    };
    
    
    int resultId = -1;
    if (SDL_ShowMessageBox(&messageBoxData, &resultId) == 0)
    {
        return static_cast<int32_t>(resultId);
    }
    return -1;
#endif
}

}
