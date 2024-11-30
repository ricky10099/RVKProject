#pragma once
#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/RVKSwapChain.h"
#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Vulkan/RVKDescriptor.h"
#include "Framework/Vulkan/RenderSystem/SimpleRenderSystem.h"

namespace RVK {
	class RVKRenderer {
	public:
		static std::unique_ptr<RVKDescriptorPool> s_descriptorPool;

	public:
		RVKRenderer(RVKWindow* window);
		~RVKRenderer();

		NO_COPY(RVKRenderer)

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void Render(VkCommandBuffer commandBuffer);
		// Getter Functions
		VkRenderPass GetRenderPass() const { return m_rvkSwapChain->GetRenderPass(); }
		float GetAspectRatio() const { return m_rvkSwapChain->GetAspectRatio(); }
		u32 GetFrameIndex() const;
		bool IsFrameInProgress() const { return m_isFrameInProgress; }

	private:
		RVKWindow* m_rvkWindow;
		std::unique_ptr<RVKSwapChain> m_rvkSwapChain;
		std::vector<VkCommandBuffer> m_commandBuffers;

		std::unique_ptr<RVKDescriptorPool> m_samplerDescriptorPool;
		std::unique_ptr<RVKDescriptorPool> m_inputDescriptorPool;

		VkDescriptorSetLayout m_descriptorSetLayout;
		//std::unique_ptr<VulkanDescriptorSetLayout> m_descriptorSetLayout;
		std::unique_ptr<RVKDescriptorSetLayout> m_samplerSetLayout;
		std::unique_ptr<RVKDescriptorSetLayout> m_inputSetLayout;

		//VkDescriptorSetLayout m_globalDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_descriptorSets{ MAX_FRAME_DRAWS };
		std::vector<VkDescriptorSet> m_samplerDescriptorSets{ MAX_FRAME_DRAWS };
		std::vector<VkDescriptorSet> m_inputDescriptorSets{ MAX_FRAME_DRAWS };

		std::vector<std::unique_ptr<RVKBuffer>> m_uniformBuffers{ MAX_FRAME_DRAWS };
		std::vector<std::unique_ptr<RVKBuffer>> m_modelBuffers{ MAX_FRAME_DRAWS };

		std::unique_ptr<SimpleRenderSystem> m_simpleRenderSystem;

		u32 m_currentImageIndex = 0;
		u32 m_currentFrameIndex = 0;
		bool m_isFrameInProgress = false;

	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void RecreateSwapChain();
	};
}