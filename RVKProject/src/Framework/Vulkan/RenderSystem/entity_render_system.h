#pragma once

#include <EnTT/entt.hpp>

#include "Framework/Vulkan/RVKPipeline.h"

namespace RVK {
	class EntityRenderSystem {
	public:
		EntityRenderSystem(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> globalSetLayouts);
		~EntityRenderSystem();

		NO_COPY(EntityRenderSystem)

		void RenderEntities(FrameInfo& frameInfo, entt::registry& registry);

	private:
		void CreatePipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout);
		void CreatePipeline(VkRenderPass renderPass);

		std::unique_ptr<RVKPipeline> m_rvkPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}  // namespace RVK
