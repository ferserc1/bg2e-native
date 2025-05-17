
#include <bg2e/render/vulkan/geo/Mesh.hpp>

namespace bg2e::render::vulkan::geo {

template <typename MeshT>
MeshGeneric<MeshT>::~MeshGeneric()
{
    cleanup();
}

template <typename MeshT>
void MeshGeneric<MeshT>::build()
{
	if (_meshData.vertices.empty() || _meshData.indices.empty()) {
		throw std::runtime_error("Mesh data is empty");
	}

	auto bufferSize = sizeof(_meshData.vertices[0]) * _meshData.vertices.size();
	auto stagingBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
		_vulkan,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY
	));

	void* dataPtr = stagingBuffer->allocatedData();

	memcpy(dataPtr, _meshData.vertices.data(), bufferSize);

	_vertexBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
		_vulkan,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	));

	auto indexBufferSize = sizeof(_meshData.indices[0]) * _meshData.indices.size();
	auto indexStagingBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
		_vulkan,
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY
	));

	dataPtr = indexStagingBuffer->allocatedData();
	memcpy(dataPtr, _meshData.indices.data(), indexBufferSize);

	_indexBuffer = std::unique_ptr<Buffer>(Buffer::createAllocatedBuffer(
		_vulkan,
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	));

	_vulkan->command().immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy copyData = {};
		copyData.dstOffset = 0;
		copyData.srcOffset = 0;
		copyData.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer->handle(), _vertexBuffer->handle(), 1, &copyData);

		copyData.size = indexBufferSize;
		vkCmdCopyBuffer(cmd, indexStagingBuffer->handle(), _indexBuffer->handle(), 1, &copyData);
		});

	stagingBuffer->cleanup();
	indexStagingBuffer->cleanup();
}

template <typename MeshT>
void MeshGeneric<MeshT>::draw(VkCommandBuffer cmd)
{
	VkBuffer vertexBuffers[] = { _vertexBuffer->handle() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmd, _indexBuffer->handle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, uint32_t(_meshData.indices.size()), 1, 0, 0, 0);
}

template <typename MeshT>
void MeshGeneric<MeshT>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex)
{
	if (submeshIndex >= _meshData.submeshes.size()) {
		throw std::runtime_error("Submesh index out of range");
	} 

	VkBuffer vertexBuffers[] = { _vertexBuffer->handle() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(cmd, _indexBuffer->handle(), 0, VK_INDEX_TYPE_UINT32);
	auto& submesh = _meshData.submeshes[submeshIndex];
	vkCmdDrawIndexed(cmd, submesh.indexCount, 1, submesh.firstIndex, 0, 0);
}

template <typename MeshT>
void MeshGeneric<MeshT>::cleanup()
{
	if (_vertexBuffer) {
		_vertexBuffer->cleanup();
		_vertexBuffer.reset();
	}
	if (_indexBuffer) {
		_indexBuffer->cleanup();
		_indexBuffer.reset();
	}
}

template MeshGeneric<bg2e::geo::MeshP>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshP& MeshGeneric<bg2e::geo::MeshP>::meshData();
template const bg2e::geo::MeshP& MeshGeneric<bg2e::geo::MeshP>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshP>::setMeshData(const bg2e::geo::MeshP& m);
template void MeshGeneric<bg2e::geo::MeshP>::setMeshData(const bg2e::geo::MeshP* m);
template uint32_t MeshGeneric<bg2e::geo::MeshP>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshP>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshP>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshP>::build();
template void MeshGeneric<bg2e::geo::MeshP>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshP>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshP>::cleanup();

template MeshGeneric<bg2e::geo::MeshPN>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPN& MeshGeneric<bg2e::geo::MeshPN>::meshData();
template const bg2e::geo::MeshPN& MeshGeneric<bg2e::geo::MeshPN>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPN>::setMeshData(const bg2e::geo::MeshPN& m);
template void MeshGeneric<bg2e::geo::MeshPN>::setMeshData(const bg2e::geo::MeshPN* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPN>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPN>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPN>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPN>::build();
template void MeshGeneric<bg2e::geo::MeshPN>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPN>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPN>::cleanup();

template MeshGeneric<bg2e::geo::MeshPC>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPC& MeshGeneric<bg2e::geo::MeshPC>::meshData();
template const bg2e::geo::MeshPC& MeshGeneric<bg2e::geo::MeshPC>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPC>::setMeshData(const bg2e::geo::MeshPC& m);
template void MeshGeneric<bg2e::geo::MeshPC>::setMeshData(const bg2e::geo::MeshPC* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPC>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPC>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPC>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPC>::build();
template void MeshGeneric<bg2e::geo::MeshPC>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPC>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPC>::cleanup();

template MeshGeneric<bg2e::geo::MeshPU>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPU& MeshGeneric<bg2e::geo::MeshPU>::meshData();
template const bg2e::geo::MeshPU& MeshGeneric<bg2e::geo::MeshPU>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPU>::setMeshData(const bg2e::geo::MeshPU& m);
template void MeshGeneric<bg2e::geo::MeshPU>::setMeshData(const bg2e::geo::MeshPU* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPU>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPU>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPU>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPU>::build();
template void MeshGeneric<bg2e::geo::MeshPU>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPU>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPU>::cleanup();

template MeshGeneric<bg2e::geo::MeshPNU>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPNU& MeshGeneric<bg2e::geo::MeshPNU>::meshData();
template const bg2e::geo::MeshPNU& MeshGeneric<bg2e::geo::MeshPNU>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPNU>::setMeshData(const bg2e::geo::MeshPNU& m);
template void MeshGeneric<bg2e::geo::MeshPNU>::setMeshData(const bg2e::geo::MeshPNU* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPNU>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNU>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNU>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPNU>::build();
template void MeshGeneric<bg2e::geo::MeshPNU>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPNU>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPNU>::cleanup();

template MeshGeneric<bg2e::geo::MeshPNC>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPNC& MeshGeneric<bg2e::geo::MeshPNC>::meshData();
template const bg2e::geo::MeshPNC& MeshGeneric<bg2e::geo::MeshPNC>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPNC>::setMeshData(const bg2e::geo::MeshPNC& m);
template void MeshGeneric<bg2e::geo::MeshPNC>::setMeshData(const bg2e::geo::MeshPNC* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPNC>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNC>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNC>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPNC>::build();
template void MeshGeneric<bg2e::geo::MeshPNC>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPNC>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPNC>::cleanup();

template MeshGeneric<bg2e::geo::MeshPNUC>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPNUC& MeshGeneric<bg2e::geo::MeshPNUC>::meshData();
template const bg2e::geo::MeshPNUC& MeshGeneric<bg2e::geo::MeshPNUC>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPNUC>::setMeshData(const bg2e::geo::MeshPNUC& m);
template void MeshGeneric<bg2e::geo::MeshPNUC>::setMeshData(const bg2e::geo::MeshPNUC* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPNUC>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUC>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUC>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPNUC>::build();
template void MeshGeneric<bg2e::geo::MeshPNUC>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPNUC>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPNUC>::cleanup();

template MeshGeneric<bg2e::geo::MeshPNUT>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPNUT& MeshGeneric<bg2e::geo::MeshPNUT>::meshData();
template const bg2e::geo::MeshPNUT& MeshGeneric<bg2e::geo::MeshPNUT>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPNUT>::setMeshData(const bg2e::geo::MeshPNUT& m);
template void MeshGeneric<bg2e::geo::MeshPNUT>::setMeshData(const bg2e::geo::MeshPNUT* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPNUT>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUT>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUT>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPNUT>::build();
template void MeshGeneric<bg2e::geo::MeshPNUT>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPNUT>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPNUT>::cleanup();

template MeshGeneric<bg2e::geo::MeshPNUUT>::MeshGeneric(Vulkan*);
template bg2e::geo::MeshPNUUT& MeshGeneric<bg2e::geo::MeshPNUUT>::meshData();
template const bg2e::geo::MeshPNUUT& MeshGeneric<bg2e::geo::MeshPNUUT>::meshData() const;
template void MeshGeneric<bg2e::geo::MeshPNUUT>::setMeshData(const bg2e::geo::MeshPNUUT& m);
template void MeshGeneric<bg2e::geo::MeshPNUUT>::setMeshData(const bg2e::geo::MeshPNUUT* m);
template uint32_t MeshGeneric<bg2e::geo::MeshPNUUT>::submeshCount() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUUT>::vertexBuffer() const;
template const Buffer* MeshGeneric<bg2e::geo::MeshPNUUT>::indexBuffer() const;
template void MeshGeneric<bg2e::geo::MeshPNUUT>::build();
template void MeshGeneric<bg2e::geo::MeshPNUUT>::draw(VkCommandBuffer cmd);
template void MeshGeneric<bg2e::geo::MeshPNUUT>::drawSubmesh(VkCommandBuffer cmd, uint32_t submeshIndex);
template void MeshGeneric<bg2e::geo::MeshPNUUT>::cleanup();

template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshP>::bindingDescription() { return bindingDescriptionP(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPC>::bindingDescription() { return bindingDescriptionPC(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPU>::bindingDescription() { return bindingDescriptionPU(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPN>::bindingDescription() { return bindingDescriptionPN(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPNU>::bindingDescription() { return bindingDescriptionPNU(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPNC>::bindingDescription() { return bindingDescriptionPNC(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPNUC>::bindingDescription() { return bindingDescriptionPNUC(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPNUT>::bindingDescription() { return bindingDescriptionPNUT(); }
template <> VkVertexInputBindingDescription MeshGeneric<bg2e::geo::MeshPNUUT>::bindingDescription() { return bindingDescriptionPNUUT(); }

template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshP>::attributeDescriptions() { return attributeDescriptionsP(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPC>::attributeDescriptions() { return attributeDescriptionsPC(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPU>::attributeDescriptions() { return attributeDescriptionsPU(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPN>::attributeDescriptions() { return attributeDescriptionsPN(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPNU>::attributeDescriptions() { return attributeDescriptionsPNU(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPNC>::attributeDescriptions() { return attributeDescriptionsPNC(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPNUC>::attributeDescriptions() { return attributeDescriptionsPNUC(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPNUT>::attributeDescriptions() { return attributeDescriptionsPNUT(); }
template <> std::vector<VkVertexInputAttributeDescription> MeshGeneric<bg2e::geo::MeshPNUUT>::attributeDescriptions() { return attributeDescriptionsPNUUT(); }


}

