#pragma once

#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/RVKSwapChain.h"

namespace RVK {
	class RVKRenderer {
	public:
		RVKRenderer(RVKWindow& window);
		~RVKRenderer();

		NO_COPY(RVKRenderer)

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);

		VkCommandBuffer GetCurrentCommandBuffer() const {
			VK_ASSERT(m_isFrameStarted, "Cannot Get Command Buffer when Frame not in progress");
			return m_commandBuffers[m_currentFrameIndex];
		}

		int GetFrameIndex() const {
			VK_ASSERT(m_isFrameStarted, "Cannot Get Frame Index when Frame not in progress");
			return m_currentFrameIndex;
		}

		VkRenderPass GetSwapChainRenderPass() const { return m_rvkSwapChain->GetRenderPass(); }
		bool IsFrameInProgress() const { return m_isFrameStarted; }
		u32 GetFrameCounter() const { return m_frameCounter; }
		float GetAspectRatio() const { return m_rvkSwapChain->ExtentAspectRatio(); }

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

		u32 m_frameCounter = 0;
	};
}  // namespace RVK
