#include "Framework/Vulkan/RenderSystem/entity_render_system.h"

#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Component.h"

namespace RVK {
	struct EntityPushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	EntityRenderSystem::EntityRenderSystem(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> globalSetLayout) {
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	EntityRenderSystem::~EntityRenderSystem() {
		vkDestroyPipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), m_pipelineLayout, nullptr);
	}

	void EntityRenderSystem::CreatePipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(EntityPushConstantData);

		//std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<u32>(globalSetLayout.size());
		pipelineLayoutInfo.pSetLayouts = globalSetLayout.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VkResult result = vkCreatePipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		VK_CHECK(result, "Failed to Create Pipeline Layout!");
	}

	void EntityRenderSystem::CreatePipeline(VkRenderPass renderPass) {
		VK_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

		PipelineConfigInfo pipelineConfig{};
		RVKPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_pipelineLayout;
		m_rvkPipeline = std::make_unique<RVKPipeline>(
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
		);
	}

	void EntityRenderSystem::RenderEntities(FrameInfo& frameInfo, entt::registry& registry) {
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

		/*auto view = registry.view<Components::Mesh, Components::Transform>();
		for (auto entity : view) {
			auto& mesh = view.get<Components::Mesh>(entity);
			auto& transform = view.get<Components::Transform>(entity);

			if (mesh.model == nullptr) continue;
			EntityPushConstantData push{};
			push.modelMatrix = mesh.offset.GetTransform() * transform.GetTransform();
			push.normalMatrix = mesh.offset.NormalMatrix() * transform.NormalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				m_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(EntityPushConstantData),
				&push);

			static_cast<Model*>(mesh.model.get())->Bind(frameInfo.commandBuffer);
			static_cast<Model*>(mesh.model.get())->Draw(frameInfo.commandBuffer);
		}*/

		auto view2 = registry.view<Components::Model, Components::Transform>();
		for (auto entity : view2) {
			auto& mesh = view2.get<Components::Model>(entity);
			auto& transform = view2.get<Components::Transform>(entity);

			if (mesh.model == nullptr) continue;
			EntityPushConstantData push{};
			push.modelMatrix = mesh.offset.GetTransform() * transform.GetTransform();
			push.normalMatrix = mesh.offset.NormalMatrix() * transform.NormalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				m_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(EntityPushConstantData),
				&push);

			static_cast<MeshModel*>(mesh.model.get())->Bind(frameInfo, m_pipelineLayout);
			static_cast<MeshModel*>(mesh.model.get())->Draw(frameInfo, m_pipelineLayout);
		}
	}
}  // namespace RVK
