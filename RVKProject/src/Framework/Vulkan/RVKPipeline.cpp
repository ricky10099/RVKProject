#include "Framework/Vulkan/RVKPipeline.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
    RVKPipeline::RVKPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfig& config) {
        CreateGraphicsPipeline(vertFilepath, fragFilepath, config);
    }

    RVKPipeline::~RVKPipeline() {
        vkDestroyShaderModule(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_vertShaderModule, nullptr);
        vkDestroyShaderModule(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_fragShaderModule, nullptr);
        vkDestroyPipeline(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_graphicsPipeline, nullptr);
        VK_CORE_INFO("Pipeline Destroyed!");
    }

    void RVKPipeline::CreateGraphicsPipeline(
        const std::string& vertFilepath,
        const std::string& fragFilepath,
        const PipelineConfig& config) {
        VK_CORE_ASSERT(config.pipelineLayout != VK_NULL_HANDLE, "Cannot create graphics pipeline: no pipelineLayout provided in config");
        VK_CORE_ASSERT(config.renderPass != VK_NULL_HANDLE, "Cannot create graphics pipeline: no renderPass provided in config");

        // Read in SPIR-V code of shaders
        auto vertCode = ReadFile(vertFilepath);
        auto fragCode = ReadFile(fragFilepath);

        // Create Shader Modules
        CreateShaderModule(vertCode, m_vertShaderModule);
        CreateShaderModule(fragCode, m_fragShaderModule);

        // -- SHADER STAGE CREATION INFORMATION --
        VkPipelineShaderStageCreateInfo shaderStages[2];
        // Vertex Stage creation information
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = m_vertShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        // Fragment Stage creation information
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = m_fragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        auto& bindingDescriptions = config.bindingDescriptions;
        auto& attributeDescriptions = config.attributeDescriptions;

        // -- VERTEX INPUT --
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();                            // List of Vertex Binding Descriptions (data spacing/ stride information)
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();                        // List of VertexAttribute Descriptions (data format and where to bind to/from)

        // -- GRAPHICS PIPELINE CREATION --
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;                                        // Number of shader stages
        pipelineInfo.pStages = shaderStages;                                // List of shader stages
        pipelineInfo.pVertexInputState = &vertexInputInfo;                  // All the fixed function pipeline states
        pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
        pipelineInfo.pViewportState = &config.viewportInfo;
        pipelineInfo.pDynamicState = &config.dynamicStateInfo;
        pipelineInfo.pRasterizationState = &config.rasterizationInfo;
        pipelineInfo.pMultisampleState = &config.multisampleInfo;
        pipelineInfo.pColorBlendState = &config.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &config.depthStencilInfo;
        pipelineInfo.layout = config.pipelineLayout;                    // Pipeline Layout pipeline should use
        pipelineInfo.renderPass = config.renderPass;                    // Render pass description the pipeline is compatible with
        pipelineInfo.subpass = config.subpass;                          // Subpass of render pass to use with pipeline

        // Pipeline Derivatives: Can create multiple pipelines that derive from onr another for optimisation
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;                   // Existing pipeline to derive from...
        pipelineInfo.basePipelineIndex = -1;                                // or index of pipeline being created to derive from (in case creating multiple at once)

        // Create Graphics Pipeline
        VkResult result = vkCreateGraphicsPipelines(RVKDevice::s_rvkDevice->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
        VK_CHECK(result, "Failed to create a Graphics Piepline!");
    }

    void RVKPipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkResult result = vkCreateShaderModule(RVKDevice::s_rvkDevice->GetLogicalDevice(), &createInfo, nullptr, &shaderModule);
        VK_CHECK(result, "Failed to create a shader module!");
    }

    void RVKPipeline::Bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    }

    void RVKPipeline::DefaultPipelineConfigInfo(PipelineConfig& config) {
        // -- INPUT ASSEMBLY --
        config.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        config.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;                        // Primitive type to assemble vertices as
        config.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        // -- VIEWPORT & SCISSOR --
        config.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        config.viewportInfo.viewportCount = 1;
        config.viewportInfo.pViewports = nullptr;
        config.viewportInfo.scissorCount = 1;
        config.viewportInfo.pScissors = nullptr;

        // -- RASTERIZER --
        config.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        config.rasterizationInfo.depthClampEnable = VK_FALSE;                   // Change if fragment beyond near/far planes are clipped (default) or clamped to plane
        config.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;            // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
        config.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;            // How to handle filling points between vertices
        config.rasterizationInfo.lineWidth = 1.0f;                              // How thick lines should be when drawn
        config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;                  // Which face of a tri to cull
        config.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;           // Windong to determine which side is front
        config.rasterizationInfo.depthBiasEnable = VK_FALSE;                    // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
        config.rasterizationInfo.depthBiasConstantFactor = 0.0f;                // Optional
        config.rasterizationInfo.depthBiasClamp = 0.0f;                         // Optional
        config.rasterizationInfo.depthBiasSlopeFactor = 0.0f;                   // Optional

        // -- MULTISAMPLING --
        config.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        config.multisampleInfo.sampleShadingEnable = VK_FALSE;                  // Enable multisample shading or not
        config.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;    // Number of samples to use per fragment
        config.multisampleInfo.minSampleShading = 1.0f;                         // Optional
        config.multisampleInfo.pSampleMask = nullptr;                           // Optional
        config.multisampleInfo.alphaToCoverageEnable = VK_FALSE;                // Optional
        config.multisampleInfo.alphaToOneEnable = VK_FALSE;                     // Optional

        // -- BLENDING --
        // Blending decides hot to blend a new colour being written to a fragment, with the old value
        // Blend Attachment State (how bkending is handled)
        config.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |        // Colors to apply blending to
            VK_COLOR_COMPONENT_A_BIT;
        config.colorBlendAttachment.blendEnable = VK_TRUE;                                      // Enable blending

        // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
        config.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;            // Optional
        config.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // Optional
        config.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;                             // Optional
        // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
        //             (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

        config.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;              // Optional
        config.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;             // Optional
        config.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;                         // Optional
        // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

        config.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        config.colorBlendInfo.logicOpEnable = VK_FALSE;                     // Alternative to calculations is to use logical operations
        config.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;                   // Optional
        config.colorBlendInfo.attachmentCount = 1;
        config.colorBlendInfo.pAttachments = &config.colorBlendAttachment;
        config.colorBlendInfo.blendConstants[0] = 0.0f;                     // Optional
        config.colorBlendInfo.blendConstants[1] = 0.0f;                     // Optional
        config.colorBlendInfo.blendConstants[2] = 0.0f;                     // Optional
        config.colorBlendInfo.blendConstants[3] = 0.0f;                     // Optional

        // -- DEPTH STENCIL TESTING --
        config.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        config.depthStencilInfo.depthTestEnable = VK_TRUE;              // Enable checking depth to determine fragment write
        config.depthStencilInfo.depthWriteEnable = VK_TRUE;             // Enable writing to depth buffer (to replace old values)
        config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;    // Comparison operation that allows an overwite (is in front)
        config.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;       // Depth Bounds Test: Does the depth value exist between two bounds
        config.depthStencilInfo.stencilTestEnable = VK_FALSE;           // Enable Stencil Test
        config.depthStencilInfo.minDepthBounds = 0.0f;                  // Optional
        config.depthStencilInfo.maxDepthBounds = 1.0f;                  // Optional
        config.depthStencilInfo.front = {};                             // Optional
        config.depthStencilInfo.back = {};                              // Optional

        // -- DYNAMIC STATES --
        // Dynamic states to enables
        config.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        config.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        config.dynamicStateInfo.pDynamicStates = config.dynamicStateEnables.data();
        config.dynamicStateInfo.dynamicStateCount = static_cast<u32>(config.dynamicStateEnables.size());
        config.dynamicStateInfo.flags = 0;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
        attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
        attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
        attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

        config.bindingDescriptions = bindingDescriptions;
        config.attributeDescriptions = attributeDescriptions;
    }
}
