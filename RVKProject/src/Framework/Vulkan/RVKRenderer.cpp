#include "Framework/Vulkan/RVKRenderer.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	RVKRenderer::RVKRenderer(RVKWindow& window)
		: m_rvkWindow{ window }{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	RVKRenderer::~RVKRenderer() { FreeCommandBuffers(); }

	void RVKRenderer::RecreateSwapChain() {
		auto extent = m_rvkWindow.GetExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = m_rvkWindow.GetExtent();
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(RVKDevice::s_rvkDevice->GetDevice());

		if (m_rvkSwapChain == nullptr) {
			m_rvkSwapChain = std::make_unique<RVKSwapChain>(extent);
		}
		else {
			std::shared_ptr<RVKSwapChain> oldSwapChain = std::move(m_rvkSwapChain);
			m_rvkSwapChain = std::make_unique<RVKSwapChain>(extent, oldSwapChain);

			if (!oldSwapChain->CompareSwapFormats(*m_rvkSwapChain.get())) {
				VK_CORE_CRITICAL("Swap Chain Image(or Depth) Format Has Changed!");
			}
		}
	}

	void RVKRenderer::CreateCommandBuffers() {
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = RVKDevice::s_rvkDevice->GetCommandPool();
		allocInfo.commandBufferCount = static_cast<u32>(m_commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(RVKDevice::s_rvkDevice->GetDevice(), &allocInfo, m_commandBuffers.data());
		VK_CHECK(result, "Failed to Allocate Command Buffers!");
	}

	void RVKRenderer::FreeCommandBuffers() {
		vkFreeCommandBuffers(
			RVKDevice::s_rvkDevice->GetDevice(),
			RVKDevice::s_rvkDevice->GetCommandPool(),
			static_cast<u32>(m_commandBuffers.size()),
			m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	VkCommandBuffer RVKRenderer::BeginFrame() {
		VK_ASSERT(!m_isFrameStarted, "Can't Call BeginFrame while already in progress!");

		auto result = m_rvkSwapChain->AcquireNextImage(&m_currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			VK_CORE_CRITICAL("Failed to Acquire Swap Chain Image!");
		}

		m_isFrameStarted = true;

		auto commandBuffer = GetCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VK_CHECK(result, "Failed to Begin Recording Command Buffer!")

		return commandBuffer;
	}

	void RVKRenderer::EndFrame() {
		VK_ASSERT(m_isFrameStarted, "Can't Call EndFrame while Frame is not in progress!");
		auto commandBuffer = GetCurrentCommandBuffer();

		VkResult result = vkEndCommandBuffer(commandBuffer);
		VK_CHECK(result, "Failed to Record Command buffer!")

		result = m_rvkSwapChain->SubmitCommandBuffers(&commandBuffer, &m_currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
			m_rvkWindow.WasWindowResized()) {
			m_rvkWindow.ResetWindowResizedFlag();
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			VK_CORE_CRITICAL("Failed to Present Swap Chain image!");
		}

		m_isFrameStarted = false;
		m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void RVKRenderer::BeginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		VK_ASSERT(m_isFrameStarted, "Can't Call BeginSwapChainRenderPass if Frame is not in progress!");
		VK_ASSERT(
			commandBuffer == GetCurrentCommandBuffer(),
			"Can't Begin Render Pass on Command Buffer from a different Frame!"
		);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_rvkSwapChain->GetRenderPass();
		renderPassInfo.framebuffer = m_rvkSwapChain->GetFrameBuffer(m_currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_rvkSwapChain->GetSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_rvkSwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(m_rvkSwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, m_rvkSwapChain->GetSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void RVKRenderer::EndSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		VK_ASSERT(m_isFrameStarted, "Can't Call EndSwapChainRenderPass if Frame is not in progress!");
		VK_ASSERT(
			commandBuffer == GetCurrentCommandBuffer(),
			"Can't End Render Pass on Command Buffer from a different Frame!");
		vkCmdEndRenderPass(commandBuffer);
	}
}  // namespace RVK
