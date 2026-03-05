//
// SubmeshWindow.cpp

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

