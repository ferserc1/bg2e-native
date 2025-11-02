//
// SubmeshWindow.cpp

#include "SubmeshWindow.hpp"
#include "AppDelegate.hpp"
#include <bg2e/ui/all.hpp>

void SubmeshWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Model Properties");
    
    setDrawFunction([&]() {

        auto stage = _appDelegate->stage();
        auto drawable = stage->isModelValid() ?
            _appDelegate->stage()->targetDrawable() :
            std::shared_ptr<bg2e::scene::Drawable>();

		if (drawable.get() != nullptr)
		{
            if (drawable.get() != _submeshSelector.editDrawable().get())
            {
                _submeshSelector.setEditDrawable(drawable);
            }
           
            if (_submeshSelector.draw())
            {
                _materialEditor.clearMaterial();
                for (auto sel : _submeshSelector.selectedItems())
                {
                    _materialEditor.addEditMaterial(drawable->renderMaterial(sel));
                }
            }

            _materialEditor.draw();
		}
    });
}

void SubmeshWindow::cleanup()
{
    _materialEditor.cleanup();
    _submeshSelector.cleanup();
}

