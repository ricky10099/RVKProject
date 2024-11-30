#pragma once
#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
    class RVKPipeline {
    public:
        RVKPipeline(
            const std::string& vertFilepath,
            const std::string& fragFilepath,
            const PipelineConfig& configInfo);
        ~RVKPipeline();

        NO_COPY(RVKPipeline)

        void Bind(VkCommandBuffer commandBuffer);

        static void DefaultPipelineConfigInfo(PipelineConfig& config);

    private:
        VkPipeline m_graphicsPipeline;
        VkShaderModule m_vertShaderModule;
        VkShaderModule m_fragShaderModule;

    private:
        void CreateGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfig& config);
        void CreateShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule);
    };
}
