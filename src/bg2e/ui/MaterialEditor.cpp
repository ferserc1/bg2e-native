//
//  MaterialEditor.cpp

#include <bg2e/ui/MaterialEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>
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
    if (_material.get() && BasicWidgets::collapsingHeader(_material->materialAttributes().name() + "'s Material Attributes"))
    {
        std::vector<std::string> uvOptions = { "Set 0", "Set 1" };
        BasicWidgets::separator("Albedo");
        auto albedoColor = _material->materialAttributes().albedo();
        auto albedoScale = _material->materialAttributes().albedoScale();
        auto albedoUVSet = _material->materialAttributes().albedoUVSet();
        if (Input::colorPicker("Color##albedo", albedoColor))
        {
            _material->materialAttributes().setAlbedo(albedoColor);
        }
        _albedoWidget.selectTexture("##albedo", [&](base::Texture* tex) {
            _material->materialAttributes().setAlbedoTexture(tex);
            _material->updateTextures();
            return _material->albedoTexture();
        });
        if (Input::vec2("Scale", albedoScale))
        {
            _material->materialAttributes().setAlbedoScale(albedoScale);
        }
        if (Input::comboBox("UV Set", uvOptions, albedoUVSet))
        {
            _material->materialAttributes().setAlbedoUVSet(albedoUVSet);
        }
        
        BasicWidgets::separator("Normal");
        auto normalScale = _material->materialAttributes().normalScale();
        auto normalUVSet = _material->materialAttributes().normalUVSet();
        _normalWidget.selectTexture("##normal", [&](base::Texture* tex) {
            _material->materialAttributes().setNormalTexture(tex);
            _material->updateTextures();
            return _material->normalTexture();
        });
        if (Input::vec2("Scale##normal", normalScale))
        {
            _material->materialAttributes().setNormalScale(normalScale);
        }
        if (Input::comboBox("UV Set##normal", uvOptions, normalUVSet))
        {
            _material->materialAttributes().setNormalUVSet(normalUVSet);
        }
        
        BasicWidgets::separator("Metallic");
        auto metallic = _material->materialAttributes().metalness();
        auto metallicScale = _material->materialAttributes().metalnessScale();
        auto metallicUVSet = _material->materialAttributes().metalnessChannel();
        if (Input::sliderFloat("Value##metallic", &metallic))
        {
            _material->materialAttributes().setMetalness(metallic);
        }
        _metallicWidget.selectTexture("##metallic", [&](base::Texture* tex) {
            _material->materialAttributes().setMetalnessTexture(tex);
            _material->updateTextures();
            return _material->metalnessTexture();
        });
        if (Input::vec2("Scale##metallic", metallicScale))
        {
            _material->materialAttributes().setMetalnessScale(metallicScale);
        }
        if (Input::comboBox("UV Set##metallic", uvOptions, metallicUVSet))
        {
            _material->materialAttributes().setMetalnessUVSet(metallicUVSet);
        }

        BasicWidgets::separator("Roughness");
        auto roughness = _material->materialAttributes().roughness();
        auto roughnessScale = _material->materialAttributes().roughnessScale();
        auto roughnessUVSet = _material->materialAttributes().roughnessChannel();
        if (Input::sliderFloat("Value##roughness", &roughness))
        {
            _material->materialAttributes().setRoughness(roughness);
        }
        _roughnessWidget.selectTexture("##roughness", [&](base::Texture* tex) {
            _material->materialAttributes().setRoughnessTexture(tex);
            _material->updateTextures();
            return _material->roughnessTexture();
        });
        if (Input::vec2("Scale##roughness", roughnessScale))
        {
            _material->materialAttributes().setRoughnessScale(roughnessScale);
        }
        if (Input::comboBox("UV Set##roughness", uvOptions, roughnessUVSet))
        {
            _material->materialAttributes().setRoughnessUVSet(roughnessUVSet);
        }
        
        BasicWidgets::separator("Fresnel Tint");
        auto fresnel = _material->materialAttributes().fresnelTint();
        if (Input::colorPicker("Color##fresnel", fresnel))
        {
            _material->materialAttributes().setFresnelTint(fresnel);
        }
        
        BasicWidgets::separator("Sheen");
        auto sheenIntensity = _material->materialAttributes().sheenIntensity();
        auto sheenColor = _material->materialAttributes().sheenColor();
        if (Input::sliderFloat("Intensity##sheenIntensity", &sheenIntensity, 0.0f, 2.0f))
        {
            _material->materialAttributes().setSheenIntensity(sheenIntensity);
        }
        if (Input::colorPicker("Color##sheenColor", sheenColor))
        {
            _material->materialAttributes().setSheenColor(sheenColor);
        }
        
        
        BasicWidgets::separator("Ambient Occlussion");
        auto aoScale = _material->materialAttributes().aoScale();
        auto aoUVSet = _material->materialAttributes().aoUVSet();
        _aoWidget.selectTexture("##ao", [&](base::Texture* tex) {
            _material->materialAttributes().setAoTexture(tex);
            _material->updateTextures();
            return _material->aoTexture();
        });
        if (Input::vec2("Scale##ao", aoScale))
        {
            _material->materialAttributes().setAoScale(aoScale);
        }
        if (Input::comboBox("UV Set##ao", uvOptions, aoUVSet))
        {
            _material->materialAttributes().setAoUVSet(aoUVSet);
        }
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
