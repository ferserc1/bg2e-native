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
#include <bg2e/plugin/ComponentRegistrar.hpp>

namespace {

class ApplicationPlugin final : public bg2e::plugin::Plugin {
public:
    bg2e::plugin::PluginInfo info() const override
    {
        bg2e::plugin::PluginInfo result;
        result.id = "org.bg2e.application.components";
        result.name = "Application components";
        result.description = "Application plugin ready for component registration";
        result.version = "1.0.0";
        result.provider = "Application developer";
        result.licenseExpression = "GPL-3.0-or-later";
        result.licenseUrl = "https://www.gnu.org/licenses/gpl-3.0.html";
        return result;
    }

    void registerComponents(bg2e::plugin::ComponentRegistrar& registrar,
        const bg2e::plugin::PluginLoadContext&) override
    {
        static_cast<void>(registrar);
        // Add registrar.registerComponent<YourComponent>(metadata) here.
        // This empty contribution list is a valid, automatically loaded plugin.
    }
};

}

BG2E_DEFINE_PLUGIN(ApplicationPlugin)
