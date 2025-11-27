//
//  MessageBox.hpp

#pragma once

#include <bg2e/common.hpp>

#include <string>
#include <vector>

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
