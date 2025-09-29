//
//  MaterialEditor.cpp

#include <bg2e/ui/MaterialEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>

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
        _albedoWidget.drawImage(42, 42);
        BasicWidgets::text("Albedo Texture", true);
        
        _normalWidget.drawImage(42, 42);
        BasicWidgets::text("Normal Texture", true);
        
        _metallicWidget.drawImage(42, 42);
        BasicWidgets::text("Metallic Texture", true);
        
        _roughnessWidget.drawImage(42, 42);
        BasicWidgets::text("Roughness Texture", true);
        
        _aoWidget.drawImage(42, 42);
        BasicWidgets::text("AO Texture", true);
        
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
