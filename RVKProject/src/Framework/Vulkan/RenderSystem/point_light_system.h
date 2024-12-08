#pragma once

#include "Framework/Vulkan/RVKPipeline.h"
#include "Framework/Vulkan/FrameInfo.h"

namespace RVK {
	class PointLightSystem {
	public:
		PointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		NO_COPY(PointLightSystem)

		void Update(FrameInfo& frameInfo, GlobalUbo& ubo);
		void Render(FrameInfo& frameInfo);

	private:
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void CreatePipeline(VkRenderPass renderPass);

		std::unique_ptr<RVKPipeline> m_rvkPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}  // namespace RVK
