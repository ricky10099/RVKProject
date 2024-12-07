#pragma once

#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/RVKSwapChain.h"

namespace RVK {
	class RVKRenderer {
	public:
		RVKRenderer(RVKWindow& window);
		~RVKRenderer();

		NO_COPY(RVKRenderer)

		VkRenderPass GetSwapChainRenderPass() const { return m_rvkSwapChain->GetRenderPass(); }
		float GetAspectRatio() const { return m_rvkSwapChain->ExtentAspectRatio(); }
		bool IsFrameInProgress() const { return m_isFrameStarted; }

		VkCommandBuffer GetCurrentCommandBuffer() const {
			VK_ASSERT(m_isFrameStarted, "Cannot Get Command Buffer when Frame not in progress");
			return m_commandBuffers[m_currentFrameIndex];
		}

		int GetFrameIndex() const {
			VK_ASSERT(m_isFrameStarted, "Cannot Get Frame Index when Frame not in progress");
			return m_currentFrameIndex;
		}

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void RecreateSwapChain();

		RVKWindow& m_rvkWindow;
		std::unique_ptr<RVKSwapChain> m_rvkSwapChain;
		std::vector<VkCommandBuffer> m_commandBuffers;

		u32 m_currentImageIndex;
		int m_currentFrameIndex{ 0 };
		bool m_isFrameStarted{ false };
	};
}  // namespace RVK
