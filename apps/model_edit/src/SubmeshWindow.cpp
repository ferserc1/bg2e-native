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

        auto stage = _appDelegate->stage();
        auto targetNames = stage->targetNames();
        auto drawable = stage->isModelValid() ?
            _appDelegate->stage()->targetDrawable() :
            std::shared_ptr<bg2e::scene::Drawable>();

		if (drawable.get() != nullptr)
		{
		    if (targetNames.size() > 0)
		    {
		        bg2e::ui::BasicWidgets::separator("Mesh");
		        bg2e::ui::SelectableList::beginList(1);


		        for (size_t i = 0; i < targetNames.size(); i++)
		        {
                    auto targetName = targetNames[i];
		            auto isSelected = i == stage->selectedTargetNodeIndex();
		            if (bg2e::ui::SelectableList::item(targetName, isSelected))
		            {
		                stage->selectTargetNode(i);
		                _materialEditor.clearMaterial();
		            }
		        }

		        bg2e::ui::SelectableList::endList();
		    }
           
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
    auto stage = _appDelegate->stage();
    auto targetNames = stage->targetNames();
    auto drawable = stage->isModelValid() ?
        _appDelegate->stage()->targetDrawable() :
        std::shared_ptr<bg2e::scene::Drawable>();
    if (drawable && drawable->submeshesCount() > submeshIndex)
    {
        _materialEditor.clearMaterial();
        _materialEditor.addEditMaterial(drawable->renderMaterial(submeshIndex));
    }
}

void SubmeshWindow::cleanup()
{
    _materialEditor.cleanup();
    _drawableEditor.cleanup();
}

