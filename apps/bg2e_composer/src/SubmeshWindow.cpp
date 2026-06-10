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
#include "SubmeshWindow.hpp"
#include "AppDelegate.hpp"
#include <bg2e/ui/all.hpp>

void SubmeshWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Model Properties");

    _materialEditor.onChanged([&]()
    {
        _appDelegate->stage()->document()->setUnsavedChanges(true);
    });

    _drawableEditor.init(delegate->selectionManager());

    _drawableEditor.onChanged([&]()
    {
        _appDelegate->stage()->document()->setUnsavedChanges(true);
    });

    _materialEditor.setSelectionManager(delegate->selectionManager());

    setDrawFunction([&]() {
        auto sm = _appDelegate->selectionManager();
        auto count = sm->selectedItems().size();

        if (count == 0)
        {
            bg2e::ui::BasicWidgets::text("No selection");
            return;
        }
        if (count > 1)
        {
            bg2e::ui::BasicWidgets::text("<multiple_selection>");
            return;
        }

        auto drawable = sm->selectedMesh();
        if (drawable != nullptr)
        {
            if (_drawableEditor.draw())
            {
                _materialEditor.clearMaterial();

                for (auto sel : _drawableEditor.selectedItems())
                {
                    _materialEditor.addEditMaterial(drawable->renderMaterial(sel));
                }
            }

            _materialEditor.draw();
        }
    });
}

void SubmeshWindow::setEditMaterial(uint32_t submeshIndex)
{
    auto drawable = _appDelegate->selectionManager()->selectedMesh();
    if (drawable && drawable->submeshesCount() > submeshIndex)
    {
        _materialEditor.clearMaterial();
        _materialEditor.addEditMaterial(drawable->renderMaterial(submeshIndex));
    }
}

void SubmeshWindow::clearMaterialSelection()
{
    _materialEditor.clearMaterial();
}

void SubmeshWindow::cleanup()
{
    _materialEditor.cleanup();
    _drawableEditor.cleanup();
}

