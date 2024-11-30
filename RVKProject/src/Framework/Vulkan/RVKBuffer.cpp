#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	RVKBuffer::RVKBuffer(VkDeviceSize instanceSize, u32 instanceCount,
		VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment)
		: m_instanceSize(instanceSize),
		m_instanceCount(instanceCount),
		m_usageFlags(usageFlags),
		m_memoryPropertyFlags(memoryPropertyFlags) {
		m_alignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
		m_bufferSize = m_alignmentSize * m_instanceCount;
		RVKDevice::s_rvkDevice->CreateBuffer(m_bufferSize, usageFlags, memoryPropertyFlags, &m_buffer, &m_memory);
	}

	RVKBuffer::~RVKBuffer() {
		Unmap();
		vkDestroyBuffer(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_buffer, nullptr);
		vkFreeMemory(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_memory, nullptr);
	}

	VkResult RVKBuffer::Map(VkDeviceSize offset, VkDeviceSize size) {
		VK_CORE_ASSERT(m_buffer && m_memory, "Called map on buffer before create");
		return vkMapMemory(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_memory, offset, size, 0, &m_mapped);
	}

	void RVKBuffer::Unmap() {
		if (m_mapped) {
			vkUnmapMemory(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_memory);
			m_mapped = nullptr;
		}
	}

	void RVKBuffer::WriteToBuffer(void* data, VkDeviceSize offset, VkDeviceSize size) {
		VK_CORE_ASSERT(m_mapped, "Cannot copy to unmapped buffer");

		if (size == VK_WHOLE_SIZE) {
			memcpy(m_mapped, data, m_bufferSize);
		}
		else {
			char* memOffset = (char*)m_mapped;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	VkResult RVKBuffer::Flush(VkDeviceSize offset, VkDeviceSize size) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(RVKDevice::s_rvkDevice->GetLogicalDevice(), 1, &mappedRange);
	}

	VkResult RVKBuffer::Invalidate(VkDeviceSize offset, VkDeviceSize size) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(RVKDevice::s_rvkDevice->GetLogicalDevice(), 1, &mappedRange);
	}

	VkDescriptorBufferInfo RVKBuffer::GetDescriptorInfo(VkDeviceSize offset, VkDeviceSize size) {
		return VkDescriptorBufferInfo{ m_buffer, offset, size };
	}

	void RVKBuffer::WriteToIndex(void* data, u32 index) {
		WriteToBuffer(data, m_instanceSize, index * m_alignmentSize);
	}

	VkResult RVKBuffer::FlushIndex(u32 index) {
		return Flush(m_instanceSize, index * m_alignmentSize);
	}

	VkDescriptorBufferInfo RVKBuffer::GetDescriptorInfoForIndex(u32 index) {
		return GetDescriptorInfo(m_instanceSize, index * m_alignmentSize);
	}

	VkResult RVKBuffer::InvalidateIndex(u32 index) {
		return Invalidate(m_instanceSize, index * m_alignmentSize);
	}


	VkDeviceSize RVKBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
		if (minOffsetAlignment > 0) {
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
		}
		return instanceSize;
	}
}