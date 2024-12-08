#pragma once

#include <vulkan/vulkan.h>

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class RVKBuffer {
	public:
		RVKBuffer(
			VkDeviceSize instanceSize,
			u32 instanceCount,
			VkBufferUsageFlags usageFlags,
			VkMemoryPropertyFlags memoryPropertyFlags,
			VkDeviceSize minOffsetAlignment = 1);
		~RVKBuffer();

		NO_COPY(RVKBuffer)

		VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void Unmap();

		void WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void WriteToIndex(void* data, int index);
		VkResult FlushIndex(int index);
		VkDescriptorBufferInfo DescriptorInfoForIndex(int index);
		VkResult InvalidateIndex(int index);

		VkBuffer GetBuffer() const { return m_buffer; }
		void* GetMappedMemory() const { return m_mapped; }
		u32 GetInstanceCount() const { return m_instanceCount; }
		VkDeviceSize GetInstanceSize() const { return m_instanceSize; }
		VkDeviceSize GetAlignmentSize() const { return m_instanceSize; }
		VkBufferUsageFlags GetUsageFlags() const { return m_usageFlags; }
		VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
		VkDeviceSize GetBufferSize() const { return m_bufferSize; }

	private:
		static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

		void* m_mapped = nullptr;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;

		VkDeviceSize m_bufferSize;
		u32 m_instanceCount;
		VkDeviceSize m_instanceSize;
		VkDeviceSize m_alignmentSize;
		VkBufferUsageFlags m_usageFlags;
		VkMemoryPropertyFlags m_memoryPropertyFlags;
	};
}  // namespace RVK
