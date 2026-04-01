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

#include <bg2e/app/GtkMessageBox.hpp>

#ifdef BG2E_LINUX
#include <gtk/gtk.h>
#endif

namespace bg2e::app::internal
{

int32_t GtkMessageBox::showMessage(const std::string& title, const std::string& message)
{
#ifdef BG2E_LINUX
    int argc = 0;
    char ** argv = nullptr;
    if (!gtk_init_check(&argc, &argv))
    {
        return -1;
    }

    GtkMessageType type = GTK_MESSAGE_INFO;
    switch (_messageType)
    {
    case MessageBox::MessageType::Info:
        type = GTK_MESSAGE_INFO;
        break;
    case MessageBox::MessageType::Warning:
        type = GTK_MESSAGE_WARNING;
        break;
    case MessageBox::MessageType::Error:
        type = GTK_MESSAGE_ERROR;
        break;
    }

    GtkWidget * dialog = gtk_message_dialog_new(
        nullptr,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        type,
        GTK_BUTTONS_NONE,
        "%s",
        message.c_str()
    );

    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());

    if (_buttons.size() == 0)
    {
        _buttons.push_back({
            .code = 0,
            .label = "Ok",
            .key = MessageBox::ButtonDefaultKey::Return
        });
    }

    for (const auto& button : _buttons)
    {
        // TODO: shortcut key?
        gtk_dialog_add_button(
            GTK_DIALOG(dialog),
            button.label.c_str(),
            button.code
        );
    }

    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(GTK_WIDGET(dialog));

    return response;
#else
    return -1;
#endif
}

}
