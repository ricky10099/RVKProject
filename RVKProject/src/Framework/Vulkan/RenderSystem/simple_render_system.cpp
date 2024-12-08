#include "Framework/Vulkan/RenderSystem/simple_render_system.h"

#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	SimpleRenderSystem::SimpleRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) {
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), m_pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

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

	void SimpleRenderSystem::CreatePipeline(VkRenderPass renderPass) {
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

	void SimpleRenderSystem::RenderGameObjects(FrameInfo& frameInfo) {
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

		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.m_model == nullptr) continue;
			SimplePushConstantData push{};
			push.modelMatrix = obj.m_transform.Mat4();
			push.normalMatrix = obj.m_transform.NormalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				m_pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			obj.m_model->Bind(frameInfo.commandBuffer);
			obj.m_model->Draw(frameInfo.commandBuffer);
		}
	}
}  // namespace RVK
