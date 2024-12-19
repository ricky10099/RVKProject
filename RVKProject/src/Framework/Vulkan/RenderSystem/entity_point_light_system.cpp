#include "Framework/Vulkan/RenderSystem/entity_point_light_system.h"

#include "Framework/Vulkan/RVKDevice.h"
//#include "Framework/Vulkan/FrameInfo.h"
#include "Framework/Component.h"

namespace RVK {
	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	EntityPointLightSystem::EntityPointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) {
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	EntityPointLightSystem::~EntityPointLightSystem() {
		vkDestroyPipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), m_pipelineLayout, nullptr);
	}

	void EntityPointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VkResult result = vkCreatePipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		VK_CHECK(result, "Failed to Create Pipeline Layout!");
	}

	void EntityPointLightSystem::CreatePipeline(VkRenderPass renderPass) {
		VK_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

		PipelineConfigInfo pipelineConfig{};
		RVKPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		RVKPipeline::EnableAlphaBlending(pipelineConfig);
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_pipelineLayout;
		m_rvkPipeline = std::make_unique<RVKPipeline>(
			"shaders/point_light.vert.spv",
			"shaders/point_light.frag.spv",
			pipelineConfig
		);
	}

	void EntityPointLightSystem::Update(FrameInfo& frameInfo, GlobalUbo& ubo, entt::registry& registry) {
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });
		int lightIndex = 0;

		auto view = registry.view<Components::PointLight, Components::Transform>();
		for (auto entity : view)
		{
			auto& pointLight = view.get<Components::PointLight>(entity);
			auto& transform = view.get<Components::Transform>(entity);

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");
			transform.position = glm::vec3(rotateLight * glm::vec4(transform.position, 1.f));

			//copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(transform.position, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(pointLight.color, pointLight.lightIntensity);

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	void EntityPointLightSystem::Render(FrameInfo& frameInfo, entt::registry& registry) {
		m_rvkPipeline->Bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);

		auto view = registry.view<Components::PointLight, Components::Transform>();
		for (auto entity : view) {
			auto& pointLight = view.get<Components::PointLight>(entity);
			auto& transform = view.get<Components::Transform>(entity);

			PointLightPushConstants push{};
			push.position = glm::vec4(transform.position, 1.f);
			push.color = glm::vec4(pointLight.color, pointLight.lightIntensity);

			push.radius = pointLight.radius;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				m_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
}  // namespace RVK
