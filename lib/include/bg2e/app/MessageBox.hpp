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

#include <bg2e/common.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace bg2e {
namespace app {

class BG2E_API MessageBox {
public:
    enum MessageType {
        Info,
        Warning,
        Error
    };
    
    enum ButtonDefaultKey {
        Esc,
        Return,
        None
    };
    
    struct Button {
        int32_t code;
        std::string label;
        ButtonDefaultKey key = None;
    };
    
    static int32_t showInfo(
        const std::string& title,
        const std::string& message,
        const std::vector<Button>& buttons
    );
    
    static int32_t showWarning(
        const std::string& title,
        const std::string& message,
        const std::vector<Button>& buttons
    );
    
    static int32_t showError(
        const std::string& title,
        const std::string& message,
        const std::vector<Button>& buttons
    );
    
    static int32_t showInfo(
        const std::string& title,
        const std::string& message
    );
    
    static int32_t showWarning(
        const std::string& title,
        const std::string& message
    );
    
    static int32_t showError(
        const std::string& title,
        const std::string& message
    );
    
    static int32_t showMessage(
        MessageType type,
        const std::string & title,
        const std::string & message,
        const std::vector<Button>& buttons
    );
};


}
}
