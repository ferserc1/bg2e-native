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
#include "ImportSettingsWindow.hpp"

#include "ImportServer.hpp"
#include "ImportSettings.hpp"

#include <bg2e/app/MessageBox.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

void ImportSettingsWindow::init(ImportServer * server, ImportSettings * settings)
{
    _server = server;
    _settings = settings;
    _portText = std::to_string(_settings->port());
    setTitle("Import Settings");
    setSize(320, 120);
    close();

    setDrawFunction([this]() { drawUI(); });
}

void ImportSettingsWindow::drawUI()
{
    const bool running = _server->isRunning();

    bg2e::ui::Value::text("Port", _portText, 6, false, running);

    bool enabled = running;
    if (bg2e::ui::Button::checkBox("Service enabled", &enabled))
    {
        if (enabled)
        {
            uint32_t port = 0;
            if (!ImportSettings::parsePort(_portText, port))
            {
                bg2e::app::MessageBox::showError(
                    "Import Service",
                    "Invalid port. Use a number between 1024 and 49151."
                );
            }
            else
            {
                auto error = _server->start(port);
                if (!error.empty())
                {
                    bg2e::app::MessageBox::showError(
                        "Import Service",
                        error + ". Try a different port."
                    );
                }
                else
                {
                    _settings->setPort(port);
                    _settings->setServiceEnabled(true);
                    _settings->save();
                }
            }
        }
        else
        {
            _server->stop();
            _settings->setServiceEnabled(false);
            _settings->save();
        }
    }

    bg2e::ui::Text::separator();
    if (running)
    {
        bg2e::ui::Text::text(
            "Listening on 127.0.0.1:" + std::to_string(_server->port())
        );
    }
    else
    {
        bg2e::ui::Text::text("Service stopped");
    }
}
