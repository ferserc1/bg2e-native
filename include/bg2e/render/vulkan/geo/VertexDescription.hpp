#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/geo/Vertex.hpp>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace geo {


template <class T>
extern BG2E_API VkVertexInputBindingDescription bindingDescription();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexP>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPN>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPC>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPU>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNU>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNC>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUC>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUT>();

template <>
extern BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUUT>();

template <class T>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexP>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPN>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPC>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPU>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNU>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNC>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUC>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUT>();

template <>
extern BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUUT>();


}
}
}
}
