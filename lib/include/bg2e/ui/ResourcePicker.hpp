/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/reflection/Property.hpp>

#include <filesystem>
#include <string>

namespace bg2e::ui {

class BG2E_API ResourcePicker {
public:
    static bool draw(
        const std::string & label,
        std::filesystem::path & value,
        const reflection::PropertyMetadata & metadata,
        bool readOnly = false
    );
};

}
