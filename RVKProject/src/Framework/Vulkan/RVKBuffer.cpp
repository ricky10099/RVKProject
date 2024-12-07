#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	/**
	 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
	 *
	 * @param instanceSize The size of an instance
	 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
	 * minUniformBufferOffsetAlignment)
	 *
	 * @return VkResult of the buffer mapping call
	 */
	VkDeviceSize RVKBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
		if (minOffsetAlignment > 0) {
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
		}
		return instanceSize;
	}

	RVKBuffer::RVKBuffer(
		VkDeviceSize instanceSize,
		u32 instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment)
		: m_instanceSize{ instanceSize }
		, m_instanceCount{ instanceCount }
		, m_usageFlags{ usageFlags }
		, m_memoryPropertyFlags{ memoryPropertyFlags } {
		m_alignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
		m_bufferSize = m_alignmentSize * instanceCount;
		RVKDevice::s_rvkDevice->CreateBuffer(m_bufferSize, usageFlags, memoryPropertyFlags, m_buffer, m_memory);
	}

	RVKBuffer::~RVKBuffer() {
		Unmap();
		vkDestroyBuffer(RVKDevice::s_rvkDevice->GetDevice(), m_buffer, nullptr);
		vkFreeMemory(RVKDevice::s_rvkDevice->GetDevice(), m_memory, nullptr);
	}

	/**
	 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	 *
	 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
	 * buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the buffer mapping call
	 */
	VkResult RVKBuffer::Map(VkDeviceSize size, VkDeviceSize offset) {
		VK_ASSERT(m_buffer && m_memory, "Called map on buffer before create");
		return vkMapMemory(RVKDevice::s_rvkDevice->GetDevice(), m_memory, offset, size, 0, &m_mapped);
	}

	/**
	 * Unmap a mapped memory range
	 *
	 * @note Does not return a result as vkUnmapMemory can't fail
	 */
	void RVKBuffer::Unmap() {
		if (m_mapped) {
			vkUnmapMemory(RVKDevice::s_rvkDevice->GetDevice(), m_memory);
			m_mapped = nullptr;
		}
	}

	/**
	 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
	 *
	 * @param data Pointer to the data to copy
	 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
	 * range.
	 * @param offset (Optional) Byte offset from beginning of mapped region
	 *
	 */
	void RVKBuffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
		VK_ASSERT(m_mapped, "Cannot Copy to Unmapped Buffer");

		if (size == VK_WHOLE_SIZE) {
			memcpy(m_mapped, data, m_bufferSize);
		}
		else {
			char* memOffset = (char*)m_mapped;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	/**
	 * Flush a memory range of the buffer to make it visible to the device
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
	 * complete buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the flush call
	 */
	VkResult RVKBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(RVKDevice::s_rvkDevice->GetDevice(), 1, &mappedRange);
	}

	/**
	 * Invalidate a memory range of the buffer to make it visible to the host
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
	 * the complete buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the invalidate call
	 */
	VkResult RVKBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(RVKDevice::s_rvkDevice->GetDevice(), 1, &mappedRange);
	}

	/**
	 * Create a buffer info descriptor
	 *
	 * @param size (Optional) Size of the memory range of the descriptor
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkDescriptorBufferInfo of specified offset and range
	 */
	VkDescriptorBufferInfo RVKBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
		return VkDescriptorBufferInfo{
			m_buffer,
			offset,
			size,
		};
	}

	/**
	 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
	 *
	 * @param data Pointer to the data to copy
	 * @param index Used in offset calculation
	 *
	 */
	void RVKBuffer::WriteToIndex(void* data, int index) {
		WriteToBuffer(data, m_instanceSize, index * m_alignmentSize);
	}

	/**
	 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
	 *
	 * @param index Used in offset calculation
	 *
	 */
	VkResult RVKBuffer::FlushIndex(int index) { return Flush(m_alignmentSize, index * m_alignmentSize); }

	/**
	 * Create a buffer info descriptor
	 *
	 * @param index Specifies the region given by index * alignmentSize
	 *
	 * @return VkDescriptorBufferInfo for instance at index
	 */
	VkDescriptorBufferInfo RVKBuffer::DescriptorInfoForIndex(int index) {
		return DescriptorInfo(m_alignmentSize, index * m_alignmentSize);
	}

	/**
	 * Invalidate a memory range of the buffer to make it visible to the host
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param index Specifies the region to invalidate: index * alignmentSize
	 *
	 * @return VkResult of the invalidate call
	 */
	VkResult RVKBuffer::InvalidateIndex(int index) {
		return Invalidate(m_alignmentSize, index * m_alignmentSize);
	}
}  // namespace RVK
