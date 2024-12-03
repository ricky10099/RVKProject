#include "Framework/Vulkan/RenderSystem/SimpleRenderSystem.h"
#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
    struct SimplePushConstantData {
        glm::mat4 modelMatrix{ 1.f };
        glm::mat4 normalMatrix{ 1.f };
    };

    SimpleRenderSystem::SimpleRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout){
        CreatePipelineLayout(globalSetLayout);
        CreatePipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDestroyPipelineLayout(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VkResult result = vkCreatePipelineLayout(RVKDevice::s_rvkDevice->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        VK_CHECK(result, "failed to create pipeline layout!");
    }

    void SimpleRenderSystem::CreatePipeline(VkRenderPass renderPass) {
        VK_ASSERT(m_pipelineLayout != nullptr, "Cannot create pipeline before pipeline layout");

        PipelineConfig pipelineConfig{};
        RVKPipeline::DefaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = m_pipelineLayout;
        m_rvkPipeline = std::make_unique<RVKPipeline>(
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig);
    }

    void SimpleRenderSystem::RenderGameObjects(FrameInfo& frameInfo) {
        m_rvkPipeline->Bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &frameInfo.descriptorSet,
            0,
            nullptr);

        for (auto& it : frameInfo.gameObjects) {
            auto& obj = it.second;
            if (obj.model == nullptr) continue;
            SimplePushConstantData push{};
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push);
            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        }
    }

}  // namespace lve
