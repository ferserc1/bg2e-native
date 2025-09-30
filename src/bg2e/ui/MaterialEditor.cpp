//
//  MaterialEditor.cpp

#include <bg2e/ui/MaterialEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/app/FileDialog.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace bg2e::ui {

MaterialEditor::~MaterialEditor()
{

}

void MaterialEditor::setEditMaterial(std::shared_ptr<render::MaterialBase>& mat)
{
    clearWidgets();
    _material = mat;
    initWidgets();
}

void MaterialEditor::clearMaterial()
{
    _material.reset();
    clearWidgets();
}
    
bool MaterialEditor::draw()
{
    if (_material.get() && BasicWidgets::beginTree(_material->materialAttributes().name() + "'s Material Attributes"))
    {
        _albedoWidget.selectTexture("Albedo Texture", [&](base::Texture* tex) {
            _material->materialAttributes().setAlbedo(tex);
            _material->updateTextures();
            return _material->albedoTexture();
        });
        
        _normalWidget.selectTexture("Normal Texture", [&](base::Texture* tex) {
            _material->materialAttributes().setNormalTexture(tex);
            _material->updateTextures();
            return _material->normalTexture();
        });
        
        _metallicWidget.selectTexture("Metallic Texture", [&](base::Texture* tex) {
            _material->materialAttributes().setMetalness(tex);
            _material->updateTextures();
            return _material->metalnessTexture();
        });

        _roughnessWidget.selectTexture("Roughness Texture", [&](base::Texture* tex) {
            _material->materialAttributes().setRoughness(tex);
            _material->updateTextures();
            return _material->roughnessTexture();
        });
        
        _aoWidget.selectTexture("AO Texture", [&](base::Texture* tex) {
            _material->materialAttributes().setAoTexture(tex);
            _material->updateTextures();
            return _material->aoTexture();
        });
        
        BasicWidgets::endTree();
    }
    return false;
}

void MaterialEditor::cleanup()
{
    clearWidgets();
    _material.reset();
}


void MaterialEditor::initWidgets()
{
    if (!_material.get())
    {
        clearWidgets();
    }
    else
    {
        _albedoWidget.setEditTexture(_material->albedoTexture());
        _normalWidget.setEditTexture(_material->normalTexture());
        _metallicWidget.setEditTexture(_material->metalnessTexture());
        _roughnessWidget.setEditTexture(_material->roughnessTexture());
        _aoWidget.setEditTexture(_material->aoTexture());
    }
    
}

void MaterialEditor::clearWidgets()
{
    _albedoWidget.cleanup();
    _normalWidget.cleanup();
    _metallicWidget.cleanup();
    _roughnessWidget.cleanup();
    _aoWidget.cleanup();
}

}
