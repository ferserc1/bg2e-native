#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/geo/Vertex.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace geo {


BG2E_API VkVertexInputBindingDescription bindingDescriptionP();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPN();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPU();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNU();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUC();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUT();

BG2E_API VkVertexInputBindingDescription bindingDescriptionPNUUT();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsP();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPN();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPU();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNU();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUC();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUT();

BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUUT();



}
}
}
}
