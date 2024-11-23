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
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexP>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPN>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPC>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPU>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNU>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNC>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUC>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUT>();

template <>
BG2E_API VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUUT>();

template <class T>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexP>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPN>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPC>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPU>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNU>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNC>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUC>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUT>();

template <>
BG2E_API std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUUT>();



}
}
}
}
