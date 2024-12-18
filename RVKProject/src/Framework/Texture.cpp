#include "Framework/Texture.h"

#define STB_IMAGE_IMPLEMENTATION    
#include <stb/stb_image.h>

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/RVKApp.h"

namespace RVK {

	Texture::Texture(bool nearestFilter)
		: m_fileName(""), m_rendererID(0), m_localBuffer(nullptr), m_type(0), m_width(0), m_height(0), m_bytesPerPixel(0),
		m_internalFormat(0), m_dataFormat(0), m_mipLevels(0), m_sRGB(false) {
		nearestFilter ? m_minFilter = VK_FILTER_NEAREST : m_minFilter = VK_FILTER_LINEAR;
		nearestFilter ? m_magFilter = VK_FILTER_NEAREST : m_magFilter = VK_FILTER_LINEAR;
		m_minFilterMip = VK_FILTER_LINEAR;
	}

	Texture::~Texture() {
		auto device = RVKDevice::s_rvkDevice->GetDevice();

		vkDestroyImage(device, m_textureImage, nullptr);
		vkDestroyImageView(device, m_imageView, nullptr);
		vkDestroySampler(device, m_sampler, nullptr);
		vkFreeMemory(device, m_textureImageMemory, nullptr);
	}

	Texture::Texture(u32 ID, int internalFormat, int dataFormat, int type)
		: m_rendererID{ ID }, m_internalFormat{ internalFormat }, m_dataFormat{ dataFormat }, m_type{ type }, m_sRGB{ false } {}

	// create texture from raw memory
	bool Texture::Init(const u32 width, const u32 height, bool sRGB, const void* data, int minFilter, int magFilter) {
		bool ok = false;
		m_fileName = "raw memory";
		m_sRGB = sRGB;
		m_localBuffer = (u8*)data;
		m_minFilter = SetFilter(minFilter);
		m_magFilter = SetFilter(magFilter);
		m_minFilterMip = SetFilterMip(minFilter);

		if (m_localBuffer) {
			m_width = width;
			m_height = height;
			m_bytesPerPixel = 4;
			ok = Create();
		}
		return ok;
	}

	// create texture from file on disk
	bool Texture::Init(const std::string& fileName, bool sRGB, bool flip) {
		bool ok = false;
		stbi_set_flip_vertically_on_load(flip);
		m_fileName = fileName;
		m_sRGB = sRGB;
		m_localBuffer = stbi_load(m_fileName.c_str(), &m_width, &m_height, &m_bytesPerPixel, 4);

		if (m_localBuffer) {
			ok = Create();
			stbi_image_free(m_localBuffer);
		}
		else {
			VK_CORE_CRITICAL("Texture: Couldn't load file {0}", fileName);
		}

		return ok;
	}

	// create texture from file in memory
	bool Texture::Init(const unsigned char* data, int length, bool sRGB) {
		bool ok = false;
		stbi_set_flip_vertically_on_load(true);
		m_fileName = "file in memory";
		m_sRGB = sRGB;
		m_localBuffer = stbi_load_from_memory(data, length, &m_width, &m_height, &m_bytesPerPixel, 4);

		if (m_localBuffer) {
			ok = Create();
			stbi_image_free(m_localBuffer);
		}
		else {
			std::cout << "Texture: Couldn't load file " << m_fileName << std::endl;
		}
		return ok;
	}

	void Texture::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = RVKDevice::s_rvkDevice->BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_textureImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		//barrier.subresourceRange.levelCount = m_mipLevels;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			VK_CRITICAL("unsupported layout transition!");
			return;
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		RVKDevice::s_rvkDevice->EndSingleTimeCommands(commandBuffer);
	}

	void Texture::CreateImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties) {
		auto device = RVKDevice::s_rvkDevice->GetDevice();
		m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;

		m_imageFormat = format;
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_width;
		imageInfo.extent.height = m_height;
		imageInfo.extent.depth = 1;
		//imageInfo.mipLevels = m_mipLevels;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		{
			auto result = vkCreateImage(device, &imageInfo, nullptr, &m_textureImage);
			if (result != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to create image!");
			}
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, m_textureImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = RVKDevice::s_rvkDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

		{
			auto result = vkAllocateMemory(device, &allocInfo, nullptr, &m_textureImageMemory);
			if (result != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to allocate image memory in 'void "
					"Texture::CreateImage'");
			}
		}

		vkBindImageMemory(device, m_textureImage, m_textureImageMemory, 0);
	}

	void Texture::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		auto device = RVKDevice::s_rvkDevice->GetDevice();

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			VK_CORE_CRITICAL("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = RVKDevice::s_rvkDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

		{
			auto result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
			if (result != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to allocate buffer memory!");
			}
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	bool Texture::Create() {
		auto device = RVKDevice::s_rvkDevice->GetDevice();

		VkDeviceSize imageSize = m_width * m_height * 4;

		if (!m_localBuffer) {
			VK_CORE_CRITICAL("failed to load texture image!");
			return false;
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, m_localBuffer, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		VkFormat format = m_sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		CreateImage(format, VK_IMAGE_TILING_OPTIMAL,
			/*VK_IMAGE_USAGE_TRANSFER_SRC_BIT |*/ VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		RVKDevice::s_rvkDevice->CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<u32>(m_width),
			static_cast<u32>(m_height), 1 /*layerCount*/
		);

		//GenerateMipmaps();

		TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		// Create a texture sampler
		// In Vulkan, textures are accessed by samplers
		// This separates sampling information from texture data.
		// This means you could have multiple sampler objects for the same
		// texture with different settings Note: Similar to the samplers
		// available with OpenGL 3.3
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = m_magFilter;
		samplerCreateInfo.minFilter = m_minFilter;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = static_cast<float>(m_mipLevels);
		samplerCreateInfo.maxAnisotropy = 4.0;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		{
			auto result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_sampler);
			if (result != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to create sampler!");
			}
		}

		// Create image view
		// Textures are not directly accessed by shaders and
		// are abstracted by image views.
		// Image views contain additional
		// information and sub resource ranges
		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = m_imageFormat;
		//view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		// A subresource range describes the set of mip levels (and array layers) that can be accessed through this image
		// view It's possible to create multiple image views for a single image referring to different (and/or overlapping)
		// ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		//view.subresourceRange.levelCount = m_mipLevels;
		view.subresourceRange.levelCount = 1;
		// The view will be based on the texture's image
		view.image = m_textureImage;

		{
			auto result = vkCreateImageView(device, &view, nullptr, &m_imageView);
			if (result != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to create image view!");
			}
		}

		m_descriptorImageInfo.sampler = m_sampler;
		m_descriptorImageInfo.imageView = m_imageView;
		m_descriptorImageInfo.imageLayout = m_imageLayout;

		// Check image handles
		if (m_textureImage == VK_NULL_HANDLE) {
			VK_CORE_ERROR("Invalid Vulkan Image Handle");
		}

		if (m_imageView == VK_NULL_HANDLE) {
			VK_CORE_ERROR("Invalid Vulkan Image View Handle");
		}

		if (m_sampler == VK_NULL_HANDLE) {
			VK_CORE_ERROR("Invalid Vulkan Sampler Handle");
		}

		return true;
	}

	void Texture::Blit(u32 x, u32 y, u32 width, u32 height, u32 bytesPerPixel, const void* data)
	{
		VK_CORE_CRITICAL("not implemented void Texture::Blit(u32 x, u32 y, u32 width, u32 height, u32 "
			"bytesPerPixel, const void* data)");
	}

	void Texture::Blit(u32 x, u32 y, u32 width, u32 height, int dataFormat, int type, const void* data)
	{
		VK_CORE_CRITICAL("not implemented void Texture::Blit(u32 x, u32 y, u32 width, u32 height, int dataFormat, "
			"int type, const void* data)");
	}

	void Texture::Resize(u32 width, u32 height)
	{
		VK_CORE_CRITICAL("not implemented void Texture::Resize(u32 width, u32 height)");
	}

	void Texture::GenerateMipmaps()
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(RVKDevice::s_rvkDevice->GetPhysicalDevice(), m_imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			VK_CORE_WARN("texture image format does not support linear blitting!");
			return;
		}

		VkCommandBuffer commandBuffer = RVKDevice::s_rvkDevice->BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_textureImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = m_width;
		int32_t mipHeight = m_height;

		for (u32 i = 1; i < m_mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
				nullptr, 0, nullptr, 1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer, m_textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_textureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, m_minFilterMip);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
				nullptr, 0, nullptr, 1, &barrier);

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
			nullptr, 0, nullptr, 1, &barrier);

		RVKDevice::s_rvkDevice->EndSingleTimeCommands(commandBuffer);
	}

	VkFilter Texture::SetFilter(int minMagFilter)
	{
		VkFilter filter = VK_FILTER_LINEAR;
		switch (minMagFilter)
		{
		case TEXTURE_FILTER_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		{
			filter = VK_FILTER_NEAREST;
			break;
		}
		}
		return filter;
	}

	VkFilter Texture::SetFilterMip(int minFilter)
	{
		VkFilter filter = VK_FILTER_LINEAR;
		switch (minFilter)
		{
		case TEXTURE_FILTER_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		{
			break;
		}
		case TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		{
			break;
		}
		{
			filter = VK_FILTER_NEAREST;
			break;
		}
		}
		return filter;
	}
} // namespace RVK
