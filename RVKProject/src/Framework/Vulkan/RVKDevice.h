#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKWindow.h"

namespace RVK {
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		u32 graphicsFamily;
		u32 presentFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool IsComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
	};

	class RVKDevice {
	public:
		static std::shared_ptr<RVKDevice> s_rvkDevice;
		VkPhysicalDeviceProperties m_properties;

	public:
		RVKDevice(RVKWindow* window);
		~RVKDevice();

		// Not copyable or movable
		NO_COPY(RVKDevice)
		NO_MOVE(RVKDevice)

		VkCommandPool GetCommandPool() { return m_commandPool; }
		VkDevice GetDevice() { return m_device; }
		VkSurfaceKHR GetSurface() { return m_surface; }
		VkQueue GetGraphicsQueue() { return m_graphicsQueue; }
		VkQueue GetPresentQueue() { return m_presentQueue; }
		SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_physicalDevice); }

		u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_physicalDevice); }
		VkFormat FindSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer Helper Functions
		void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CopyBufferToImage(
			VkBuffer buffer, VkImage image, u32 width, u32 height, u32 layerCount);

		void CreateImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

		// helper functions
		bool IsDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		//void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void HasGflwRequiredInstanceExtensions();
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		RVKWindow* m_rvkWindow;
		VkCommandPool m_commandPool;

		VkDevice m_device;
		VkSurfaceKHR m_surface;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;
	};
}  // namespace RVK
