#include "Framework/RVKApp.h"

namespace RVK {
    RVKApp::RVKApp(u32 width, u32 height, const std::string& title)
        : m_rvkWindow(width, height, title),
        m_rvkRenderer(&m_rvkWindow) {
        VK_CORE_INFO("VulkanApp init");
    }

    RVKApp::~RVKApp() {
        vkDeviceWaitIdle(RVKDevice::s_rvkDevice->GetLogicalDevice());
        VK_CORE_INFO("VulkanApp shutdown");
    }

    bool RVKApp::StartFrame() {
        return !m_rvkWindow.ShouldClose();
    }

    void RVKApp::EndFrame() {
        vkDeviceWaitIdle(RVKDevice::s_rvkDevice->GetLogicalDevice());
        auto commandBuffer = m_rvkRenderer.BeginFrame();
        m_rvkRenderer.BeginSwapChainRenderPass(commandBuffer);
        m_rvkRenderer.Render(commandBuffer);
        m_rvkRenderer.EndSwapChainRenderPass(commandBuffer);
        m_rvkRenderer.EndFrame();
    }
}