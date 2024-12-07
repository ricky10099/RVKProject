#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	class RVKSwapChain {
	public:
		RVKSwapChain(VkExtent2D windowExtent);
		RVKSwapChain(VkExtent2D windowExtent, std::shared_ptr<RVKSwapChain> previous);

		~RVKSwapChain();

		NO_COPY(RVKSwapChain)

		VkFramebuffer GetFrameBuffer(int index) { return m_swapChainFramebuffers[index]; }
		VkRenderPass GetRenderPass() { return m_renderPass; }
		VkImageView GetImageView(int index) { return m_swapChainImageViews[index]; }
		size_t ImageCount() { return m_swapChainImages.size(); }
		VkFormat GetSwapChainImageFormat() { return m_swapChainImageFormat; }
		VkExtent2D GetSwapChainExtent() { return m_swapChainExtent; }
		u32 Width() { return m_swapChainExtent.width; }
		u32 Height() { return m_swapChainExtent.height; }

		float ExtentAspectRatio() {
			return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);
		}
		VkFormat FindDepthFormat();

		VkResult AcquireNextImage(u32* imageIndex);
		VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, u32* imageIndex);

		bool CompareSwapFormats(const RVKSwapChain& swapChain) const {
			return swapChain.m_swapChainDepthFormat == m_swapChainDepthFormat &&
				swapChain.m_swapChainImageFormat == m_swapChainImageFormat;
		}

	private:
		void Init();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(
			const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat m_swapChainImageFormat;
		VkFormat m_swapChainDepthFormat;
		VkExtent2D m_swapChainExtent;

		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		VkRenderPass m_renderPass;

		std::vector<VkImage> m_depthImages;
		std::vector<VkDeviceMemory> m_depthImageMemorys;
		std::vector<VkImageView> m_depthImageViews;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		VkExtent2D m_windowExtent;

		VkSwapchainKHR m_swapChain;
		std::shared_ptr<RVKSwapChain> m_oldSwapChain;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;
		size_t m_currentFrame = 0;
	};
}  // namespace RVK
