#pragma once

#include <EnTT/entt.hpp>

#include "Framework/Vulkan/RVKPipeline.h"
#include "Framework/Vulkan/FrameInfo.h"

namespace RVK {
	class EntityPointLightSystem {
	public:
		EntityPointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~EntityPointLightSystem();

		NO_COPY(EntityPointLightSystem)

		void Update(FrameInfo& frameInfo, GlobalUbo& ubo, entt::registry& registry);
		void Render(FrameInfo& frameInfo, entt::registry& registry);

	private:
		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void CreatePipeline(VkRenderPass renderPass);

		std::unique_ptr<RVKPipeline> m_rvkPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}  // namespace RVK
