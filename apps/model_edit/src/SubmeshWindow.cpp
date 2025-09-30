//
// SubmeshWindow.cpp

#include "SubmeshWindow.hpp"
#include "AppDelegate.hpp"
#include <bg2e/ui/all.hpp>

void SubmeshWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Submesh");
    
    setDrawFunction([&]() {
        auto drawable = _appDelegate->stage()->targetDrawable();

		if (drawable)
		{
            std::shared_ptr<bg2e::render::MaterialBase> editMaterial;
            uint32_t selectedPlist = _appDelegate->stage()->targetSubmeshIndex();
            if (bg2e::ui::Input::comboBox("##submesh",
                [&](std::vector<std::string>& items) {
                    for (uint32_t i = 0; i < drawable->submeshesCount(); ++i)
                    {
                        items.push_back(drawable->submeshName(i));
                    }
                },
                selectedPlist,
                true, true)
            ) {
				_appDelegate->stage()->setTargetSubmeshIndex(selectedPlist);
                _materialEditor.setEditMaterial(drawable->renderMaterial(selectedPlist));
            }
            
			auto submeshIndex = _appDelegate->stage()->targetSubmeshIndex();
			drawSubmeshEditor(drawable, submeshIndex);

			drawMaterialEditor(drawable, submeshIndex);
   
            // TODO: When a new file is opened, the materialEditor must be cleared so that it releases
            // the pointer to the previous object's material.
            
            // Ensure that the material editor contains a valid editing material
            if (_materialEditor.editMaterial().get() == nullptr)
            {
                _materialEditor.setEditMaterial(drawable->renderMaterial(selectedPlist));
            }
		}
    });
}

void SubmeshWindow::cleanup()
{
    _materialEditor.cleanup();
}

void SubmeshWindow::drawSubmeshEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex)
{
	using namespace bg2e::ui;
	auto submeshName = drawable->submeshName(submeshIndex);
	auto submeshGroupName = drawable->submeshGroupName(submeshIndex);
	auto submeshVisibility = drawable->submeshVisibility(submeshIndex);
	if (Input::text("Name", submeshName, 100))
	{
		drawable->setSubmeshName(submeshName, submeshIndex);
	}

	if (Input::text("Group Name", submeshGroupName, 100))
	{
		drawable->setSubmeshGroupName(submeshGroupName, submeshIndex);
	}

	if (BasicWidgets::checkBox("Visible", &submeshVisibility))
	{
		drawable->setSubmeshVisibility(submeshVisibility, submeshIndex);
	}
}

void SubmeshWindow::drawMaterialEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex)
{
	using namespace bg2e::ui;
	BasicWidgets::text("Material: ");
    _materialEditor.draw();
}
