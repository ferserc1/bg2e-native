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

#include <bg2e/plugin/Plugin.hpp>

#include "PluginComponents.hpp"

namespace tutorial {

class TutorialPlugin final : public bg2e::plugin::Plugin {
public:
    bg2e::plugin::PluginInfo info() const override
    {
        bg2e::plugin::PluginInfo result;
        result.id = "org.bg2e.tutorial.plugins.components";
        result.name = "Reflection tutorial components";
        result.description = "Rotation and reflection samples supplied by an automatically discovered plugin";
        result.version = "1.0.0";
        result.provider = "bg2 engine";
        result.licenseExpression = "GPL-3.0-or-later";
        result.licenseUrl = "https://www.gnu.org/licenses/gpl-3.0.html";
        result.metadata.emplace("tutorial", "plugins");
        return result;
    }

    void registerComponents(bg2e::plugin::ComponentRegistrar& registrar,
        const bg2e::plugin::PluginLoadContext&) override
    {
        registerRotation(registrar);
        registerSamples(registrar);
    }
};

}

BG2E_DEFINE_PLUGIN(tutorial::TutorialPlugin)
