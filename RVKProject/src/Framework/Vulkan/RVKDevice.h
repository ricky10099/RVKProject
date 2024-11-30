#pragma once
#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/VKValidate.h"

namespace RVK {
	class RVKDevice {
	public:
		RVKDevice(RVKWindow* window);
		~RVKDevice();

		NO_COPY(RVKDevice)
		NO_MOVE(RVKDevice)

		VkCommandBuffer BeginCommandBuffer();
		void EndCommandBuffer(VkCommandBuffer& commandBuffer);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
		void CopyImageBuffer(VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		u32 FindMemoryType(u32 allowedTypes, VkMemoryPropertyFlags properties);
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		void CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags,
			VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
		VkImage CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

		// Getter Functions
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
		inline VkDevice GetLogicalDevice() const { return m_device; }
		inline VkSurfaceKHR GetSurface() const { return m_surface; }
		inline VkCommandPool GetCommandPool() const { return m_commandPool; }
		inline VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
		inline VkQueue GetPresentQueue() const { return m_presentQueue; }
		inline SwapChainDetails GetSwapChainDetails() { return m_swapChainDetails; }
		inline QueueFamilyIndices& GetQueueFamilies() { return m_queueFamilyIndices; }
		inline u32 GetGraphicsQueueFamily() const { return m_queueFamilyIndices.graphicsFamily; }

		static std::shared_ptr<RVKDevice> s_rvkDevice;

	private:
		RVKWindow* m_rvkWindow;

		// Vulkan Components
		VkInstance m_instance = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties m_deviceProperties;
		QueueFamilyIndices m_queueFamilyIndices;
		SwapChainDetails m_swapChainDetails;

		bool m_destroyed = false;

	private:
		// Vulkan Functions
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

		// Checker Functions
		bool CheckDeviceSuitable(VkPhysicalDevice device);
		bool CheckValidationLayerSupport();
		bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		// Finder Functions
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainDetails FindSwapChainDetails(VkPhysicalDevice device);
	};
}