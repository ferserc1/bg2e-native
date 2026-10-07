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

#include <bg2e/ui/UvMapPreview.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/render/Texture.hpp>

namespace bg2e {
namespace ui {

UvMapPreview::UvMapPreview() = default;

UvMapPreview::~UvMapPreview()
{
    cleanup();
}

void UvMapPreview::init(render::Engine * engine, uint32_t resolution)
{
    _engine = engine;
    _renderer = std::make_unique<render::UvMapPreviewRenderer>(engine, resolution);
    _imageWidget.init(engine);
    _dirty = true;
}

void UvMapPreview::setMesh(std::shared_ptr<geo::Mesh> mesh)
{
    if (_mesh == mesh)
    {
        return;
    }
    _mesh = mesh;
    _dirty = true;
}

void UvMapPreview::setUvSet(uint32_t uvSet)
{
    uvSet = uvSet > 1 ? 1 : uvSet;
    if (_uvSet != uvSet)
    {
        _uvSet = uvSet;
        _dirty = true;
    }
}

void UvMapPreview::setResolution(uint32_t resolution)
{
    if (_renderer != nullptr && _renderer->resolution() != resolution)
    {
        // Drop the sampled descriptor before the renderer recreates its target
        _imageWidget.clearTexture();
        _renderer->setResolution(resolution);
        _dirty = true;
    }
}

uint32_t UvMapPreview::resolution() const
{
    return _renderer != nullptr ? _renderer->resolution() : 0;
}

void UvMapPreview::refresh()
{
    _dirty = true;
}

void UvMapPreview::updatePreview()
{
    // Release the descriptor before render() replaces the sampled image
    _imageWidget.clearTexture();
    _validation = geo::UvAtlasValidator::validate(*_mesh, _uvSet);
    _renderer->render(*_mesh, _uvSet);
    // Non-owning handle: _renderer outlives the widget texture reference
    // (cleanup() clears _imageWidget before _renderer is destroyed)
    _imageWidget.setDeferredTexture(std::shared_ptr<render::Texture>(
        _renderer->texture(), [](render::Texture *) {}));
    _dirty = false;
}

void UvMapPreview::draw()
{
    if (_mesh == nullptr || _renderer == nullptr)
    {
        Text::text("No target mesh");
        return;
    }

    int uvSetValue = static_cast<int>(_uvSet);
    if (Button::radioButton("UV1", &uvSetValue, 0) ||
        Button::radioButton("UV2", &uvSetValue, 1, true))
    {
        setUvSet(static_cast<uint32_t>(uvSetValue));
    }

    if (_dirty)
    {
        updatePreview();
    }

    if (_validation.valid)
    {
        Text::text("Valid atlas - coverage: " + std::to_string(_validation.coverage * 100.0f) + " %");
    }
    else
    {
        Text::text("Invalid atlas: " + _validation.message);
    }

    _imageWidget.drawImage(_displaySize, _displaySize);
}

void UvMapPreview::cleanup()
{
    _imageWidget.cleanup();
    _renderer.reset();
}

}
}
