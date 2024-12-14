#pragma once

#include "Framework/Vulkan/RVKPipeline.h"
#include "Framework/Vulkan/FrameInfo.h"

namespace RVK {
	class PBRRenderSystem {
	public:
		PBRRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PBRRenderSystem();

		NO_COPY(PBRRenderSystem)

		void RenderGameObjects(FrameInfo& frameInfo);

	private:
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void CreatePipeline(VkRenderPass renderPass);

		std::unique_ptr<RVKPipeline> m_rvkPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}  // namespace RVK
