//
// SubmeshWindow.cpp

#include "SubmeshWindow.hpp"
#include "AppDelegate.hpp"
#include <bg2e/ui/all.hpp>

SubmeshWindow::SubmeshWindow(AppDelegate* delegate)
	: ToolWindow(Left, delegate) {
}

void SubmeshWindow::init(uint32_t uiWidth, uint32_t uiHeight)
{
    ToolWindow::init(uiWidth, uiHeight);

	_window.setTitle("Submesh");
}

void SubmeshWindow::draw()
{
	using namespace bg2e;

	_window.draw([&]() {
		auto drawable = _appDelegate->stage()->targetDrawable();

		if (drawable)
		{
			auto submeshIndex = _appDelegate->stage()->targetSubmeshIndex();
			drawSubmeshEditor(drawable, submeshIndex);

			drawMaterialEditor(drawable, submeshIndex);
		}
	});
}

void SubmeshWindow::drawSubmeshEditor(bg2e::scene::Drawable* drawable, uint32_t submeshIndex)
{
	using namespace bg2e::ui;
	BasicWidgets::text("Submesh: ");
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
}
