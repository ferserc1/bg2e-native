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

#include <bg2e/draw/RenderLoopDelegate.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/ui/UserInterface.hpp>

#include <utility>

namespace bg2e::app {

class Application {
  public:
    virtual void init(int argc, char** argv) = 0;

    inline std::shared_ptr<draw::RenderLoopDelegate>& renderDelegate()
    {
        return _renderDelegate;
    }
    inline std::shared_ptr<app::InputDelegate>& inputDelegate()
    {
        return _inputDelegate;
    }
    inline std::shared_ptr<ui::UserInterfaceDelegate>& uiDelegate()
    {
        return _uiDelegate;
    }

    inline void setRenderDelegate(std::shared_ptr<draw::RenderLoopDelegate> delegate)
    {
        _renderDelegate = std::move(delegate);
    }
    inline void setInputDelegate(std::shared_ptr<app::InputDelegate> delegate)
    {
        _inputDelegate = std::move(delegate);
    }
    inline void setUiDelegate(std::shared_ptr<ui::UserInterfaceDelegate> delegate)
    {
        _uiDelegate = std::move(delegate);
    }

  protected:
    std::shared_ptr<draw::RenderLoopDelegate> _renderDelegate;
    std::shared_ptr<app::InputDelegate> _inputDelegate;
    std::shared_ptr<ui::UserInterfaceDelegate> _uiDelegate;
};

}
