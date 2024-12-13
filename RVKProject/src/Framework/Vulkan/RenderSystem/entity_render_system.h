#pragma once

#include <EnTT/entt.hpp>

#include "Framework/Vulkan/RVKPipeline.h"
#include "Framework/Vulkan/FrameInfo.h"

namespace RVK {
	class EntityRenderSystem {
	public:
		EntityRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~EntityRenderSystem();

		NO_COPY(EntityRenderSystem)

		void RenderEntities(FrameInfo& frameInfo, entt::registry& registry);

	private:
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void CreatePipeline(VkRenderPass renderPass);

		std::unique_ptr<RVKPipeline> m_rvkPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}  // namespace RVK
