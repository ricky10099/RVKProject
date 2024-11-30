#pragma once

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class RVKSwapChain {
	public:
		RVKSwapChain(VkExtent2D extent);
		RVKSwapChain(VkExtent2D extent, std::shared_ptr<RVKSwapChain> oldSwapChain);
		~RVKSwapChain();

		NO_COPY(RVKSwapChain)

		VkResult AcquireNextImage(u32& imageIndex);
		VkResult SubmitCommandBuffers(const VkCommandBuffer& commandBuffer, const u32& imageIndex);
		bool CompareSwapFormats(const RVKSwapChain& other) const;

		// Getter Functions
		VkFramebuffer GetFramebuffer(u32 index) { return m_framebuffers[index]; }
		VkRenderPass GetRenderPass() const { return m_renderPass; }
		VkImageView GetSwapChainImageView(u32 index) { return m_swapChainImageViews[index]; }
		size_t GetImageCount() { return m_swapChainImages.size(); }
		VkFormat GetImageFormat() const { return m_imageFormat; }
		VkImageView GetColorImageView(u32 index) { return m_colorImageViews[index]; }
		VkImageView GetDepthImageView(u32 index) { return m_depthImageViews[index]; }
		VkExtent2D GetExtent() const { return m_swapChainExtent; }
		u32 GetWidth() const { return m_swapChainExtent.width; }
		u32 GetHeight() const { return m_swapChainExtent.height; }
		float GetAspectRatio() const { return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height); }

	private:
		VkFormat m_imageFormat;
		VkFormat m_depthFormat;
		VkExtent2D m_swapChainExtent;
		VkExtent2D m_windowExtent;

		std::vector<VkFramebuffer> m_framebuffers;
		VkRenderPass m_renderPass;

		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		std::vector<VkImage> m_colorImages;
		std::vector<VkDeviceMemory> m_colorImageMemorys;
		std::vector<VkImageView> m_colorImageViews;

		std::vector<VkImage> m_depthImages;
		std::vector<VkDeviceMemory> m_depthImageMemorys;
		std::vector<VkImageView> m_depthImageViews;

		VkSwapchainKHR	m_swapChain;
		std::shared_ptr<RVKSwapChain> m_oldSwapChain;

		std::vector<VkSemaphore> m_imageAvailable;
		std::vector<VkSemaphore> m_renderFinished;
		std::vector<VkFence> m_imagesInFlight;
		std::vector<VkFence> m_drawFences;
		size_t m_currentFrame = 0;

	private:
		void CreateSwapChain();
		void CreateDepthBufferImage();
		void CreateColourBufferImage();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSynchronisation();

		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> formats);
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> presentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}