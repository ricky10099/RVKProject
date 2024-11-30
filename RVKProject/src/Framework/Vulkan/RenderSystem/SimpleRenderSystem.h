#pragma once

#include <stb/stb_image.h>

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKPipeline.h"

namespace RVK {
    class SimpleRenderSystem {
    public:
        SimpleRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~SimpleRenderSystem();

        NO_COPY(SimpleRenderSystem)

        void RenderGameObjects(FrameInfo& frameInfo);

    private:
        void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void CreatePipeline(VkRenderPass renderPass);

        std::unique_ptr<RVKPipeline> m_rvkPipeline;
        VkPipelineLayout m_pipelineLayout;
    };
}
