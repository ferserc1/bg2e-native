//
//  GPUProcess.hpp
#pragma once

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>

#include <vector>

namespace bg2e {
namespace render {

class BG2E_API GPUProcess {
public:
    GPUProcess(Engine * engine);
    
    void addBinding(
        uint32_t binding,
        vulkan::Image * img,
        VkDescriptorType type,
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    void addBinding(
        uint32_t binding,
        vulkan::Buffer * buffer,
        VkDescriptorType type,
        uint32_t bufferSize,
        uint32_t bufferOffset
    );
    
    void executeShader(const std::string& shaderFile, VkExtent2D imageExtent);
    
protected:
    Engine * _engine;

    union BindingPtr {
        vulkan::Image * image;
        vulkan::Buffer * buffer;
    };
    
    enum BindingType {
        TypeImage = 0,
        TypeBuffer = 1
    };
    
    struct BindingInfo {
        uint32_t index;
        BindingPtr data;
        BindingType type;
        VkDescriptorType descriptorType;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t bufferSize;
        uint32_t bufferOffset;
    };
    
    std::vector<BindingInfo> _bindings;
};

}
}
