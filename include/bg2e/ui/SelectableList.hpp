//
//  Selectable.hpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

#pragma once

#include <bg2e/common.hpp>

#include <vector>

namespace bg2e {
namespace ui {

class BG2E_API SelectableList {
public:
    static void beginList(int columns = 1);
    static bool item(const std::string & title, bool & selected);
    static bool itemButton(const std::string & title);
    static void endList();
};

}
}
