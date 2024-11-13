#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace ui {

class BG2E_API BasicWidgets {
public:
    static void text(const std::string & text, bool sameLine = false);
    static void separator(const std::string & title = "", bool sameLine = false);
    static void listItem(const std::string & label, bool sameLine = false);
};

}
}