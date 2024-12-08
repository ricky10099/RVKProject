#include "Framework/Vulkan/RenderSystem/point_light_system.h"

#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/FrameInfo.h"

namespace RVK {
	struct PointLightPushConstants {
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	PointLightSystem::PointLightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) {
		CreatePipelineLayout(globalSetLayout);
		CreatePipeline(renderPass);
	}

	PointLightSystem::~PointLightSystem() {
		vkDestroyPipelineLayout(RVKDevice::s_rvkDevice->GetDevice(), m_pipelineLayout, nullptr);
	}

	void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
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

	void PointLightSystem::CreatePipeline(VkRenderPass renderPass) {
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

	void PointLightSystem::Update(FrameInfo& frameInfo, GlobalUbo& ubo) {
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.m_pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

			// update light position
			obj.m_transform.translation = glm::vec3(rotateLight * glm::vec4(obj.m_transform.translation, 1.f));

			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(obj.m_transform.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.m_color, obj.m_pointLight->lightIntensity);

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	void PointLightSystem::Render(FrameInfo& frameInfo) {
		// sort lights
		std::map<float, u32> sorted;
		for (auto& kv : frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.m_pointLight == nullptr) continue;

			// calculate distance
			auto offset = frameInfo.camera.GetPosition() - obj.m_transform.translation;
			float disSquared = glm::dot(offset, offset);
			sorted[disSquared] = obj.GetId();
		}

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

		// iterate through sorted lights in reverse order
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
			// use game obj id to find light object
			auto& obj = frameInfo.gameObjects.at(it->second);

			PointLightPushConstants push{};
			push.position = glm::vec4(obj.m_transform.translation, 1.f);
			push.color = glm::vec4(obj.m_color, obj.m_pointLight->lightIntensity);
			push.radius = obj.m_transform.scale.x;

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
