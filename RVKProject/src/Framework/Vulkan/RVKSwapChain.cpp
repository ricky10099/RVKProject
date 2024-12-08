#include "Framework/Vulkan/RVKSwapChain.h"

namespace RVK {
	RVKSwapChain::RVKSwapChain(VkExtent2D extent)
		:m_windowExtent{ extent } {
		Init();
	}

	RVKSwapChain::RVKSwapChain(VkExtent2D extent, std::shared_ptr<RVKSwapChain> previous)
		:m_windowExtent{ extent }, m_oldSwapChain{ previous } {
		Init();
		m_oldSwapChain = nullptr;
	}

	void RVKSwapChain::Init() {
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDepthResources();
		CreateFramebuffers();
		CreateSyncObjects();
	}

	RVKSwapChain::~RVKSwapChain() {
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(RVKDevice::s_rvkDevice->GetDevice(), imageView, nullptr);
		}
		m_swapChainImageViews.clear();

		if (m_swapChain != nullptr) {
			vkDestroySwapchainKHR(RVKDevice::s_rvkDevice->GetDevice(), m_swapChain, nullptr);
			m_swapChain = nullptr;
		}

		for (int i = 0; i < m_depthImages.size(); i++) {
			vkDestroyImageView(RVKDevice::s_rvkDevice->GetDevice(), m_depthImageViews[i], nullptr);
			vkDestroyImage(RVKDevice::s_rvkDevice->GetDevice(), m_depthImages[i], nullptr);
			vkFreeMemory(RVKDevice::s_rvkDevice->GetDevice(), m_depthImageMemorys[i], nullptr);
		}

		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(RVKDevice::s_rvkDevice->GetDevice(), framebuffer, nullptr);
		}

		vkDestroyRenderPass(RVKDevice::s_rvkDevice->GetDevice(), m_renderPass, nullptr);

		// cleanup synchronization objects
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(RVKDevice::s_rvkDevice->GetDevice(), m_renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(RVKDevice::s_rvkDevice->GetDevice(), m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(RVKDevice::s_rvkDevice->GetDevice(), m_inFlightFences[i], nullptr);
		}
	}

	VkResult RVKSwapChain::AcquireNextImage(u32* imageIndex) {
		vkWaitForFences(
			RVKDevice::s_rvkDevice->GetDevice(),
			1,
			&m_inFlightFences[m_currentFrame],
			VK_TRUE,
			std::numeric_limits<u64>::max());

		VkResult result = vkAcquireNextImageKHR(
			RVKDevice::s_rvkDevice->GetDevice(),
			m_swapChain,
			std::numeric_limits<u64>::max(),
			m_imageAvailableSemaphores[m_currentFrame],  // must be a not signaled semaphore
			VK_NULL_HANDLE,
			imageIndex);

		return result;
	}

	VkResult RVKSwapChain::SubmitCommandBuffers(const VkCommandBuffer* buffers, u32* imageIndex) {
		if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(RVKDevice::s_rvkDevice->GetDevice(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(RVKDevice::s_rvkDevice->GetDevice(), 1, &m_inFlightFences[m_currentFrame]);

		VkResult result = vkQueueSubmit(RVKDevice::s_rvkDevice->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);
		VK_CHECK(result, "Failed to Submit Draw Command Buffer!");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		result = vkQueuePresentKHR(RVKDevice::s_rvkDevice->GetPresentQueue(), &presentInfo);

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	void RVKSwapChain::CreateSwapChain() {
		SwapChainSupportDetails swapChainSupport = RVKDevice::s_rvkDevice->GetSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = RVKDevice::s_rvkDevice->GetSurface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = RVKDevice::s_rvkDevice->FindPhysicalQueueFamilies();
		u32 queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_swapChain;

		VkResult result = vkCreateSwapchainKHR(RVKDevice::s_rvkDevice->GetDevice(), &createInfo, nullptr, &m_swapChain);
		VK_CHECK(result, "Failed to Create Swap Chain!");

		// we only specified a minimum number of images in the swap chain, so the implementation is
		// allowed to create a swap chain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR(RVKDevice::s_rvkDevice->GetDevice(), m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(RVKDevice::s_rvkDevice->GetDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	void RVKSwapChain::CreateImageViews() {
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(RVKDevice::s_rvkDevice->GetDevice(), &viewInfo, nullptr, &m_swapChainImageViews[i]);
			VK_CHECK(result, "Failed to Create Texture Image View!");
		}
	}

	void RVKSwapChain::CreateRenderPass() {
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = GetSwapChainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.dstSubpass = 0;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult result = vkCreateRenderPass(RVKDevice::s_rvkDevice->GetDevice(), &renderPassInfo, nullptr, &m_renderPass);
		VK_CHECK(result, "Failed to Create Render Pass!");
	}

	void RVKSwapChain::CreateFramebuffers() {
		m_swapChainFramebuffers.resize(ImageCount());
		for (size_t i = 0; i < ImageCount(); i++) {
			std::array<VkImageView, 2> attachments = { m_swapChainImageViews[i], m_depthImageViews[i] };

			VkExtent2D swapChainExtent = GetSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(RVKDevice::s_rvkDevice->GetDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]);
			VK_CHECK(result, "Failed to Create Framebuffer!");
		}
	}

	void RVKSwapChain::CreateDepthResources() {
		VkFormat depthFormat = FindDepthFormat();
		m_swapChainDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = GetSwapChainExtent();

		m_depthImages.resize(ImageCount());
		m_depthImageMemorys.resize(ImageCount());
		m_depthImageViews.resize(ImageCount());

		for (int i = 0; i < m_depthImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			RVKDevice::s_rvkDevice->CreateImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_depthImages[i],
				m_depthImageMemorys[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(RVKDevice::s_rvkDevice->GetDevice(), &viewInfo, nullptr, &m_depthImageViews[i]);
			VK_CHECK(result, "Failed to Create Texture Image View!");
		}
	}

	void RVKSwapChain::CreateSyncObjects() {
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_imagesInFlight.resize(ImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(RVKDevice::s_rvkDevice->GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateSemaphore(RVKDevice::s_rvkDevice->GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(RVKDevice::s_rvkDevice->GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
				VK_CORE_CRITICAL("failed to create synchronization objects for a frame!");
			}
		}
	}

	VkSurfaceFormatKHR RVKSwapChain::ChooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR RVKSwapChain::ChooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				VK_CORE_INFO("Present mode: Mailbox");
				return availablePresentMode;
			}
		}

		// for (const auto &availablePresentMode : availablePresentModes) {
		//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
		//     std::cout << "Present mode: Immediate" << std::endl;
		//     return availablePresentMode;
		//   }
		// }

		VK_CORE_INFO("Present mode: V-Sync");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D RVKSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = m_windowExtent;
			actualExtent.width = std::max(
				capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(
				capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VkFormat RVKSwapChain::FindDepthFormat() {
		return RVKDevice::s_rvkDevice->FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}  // namespace RVK
