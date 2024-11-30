#include "Framework/Vulkan/RVKSwapChain.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
    RVKSwapChain::RVKSwapChain(VkExtent2D extent) : RVKSwapChain(extent, VK_NULL_HANDLE) {}

    RVKSwapChain::RVKSwapChain(VkExtent2D extent, std::shared_ptr<RVKSwapChain> oldSwapChain)
        :m_windowExtent(extent), m_oldSwapChain(oldSwapChain) {
        CreateSwapChain();
        CreateRenderPass();
        CreateColourBufferImage();
        CreateDepthBufferImage();
        CreateFramebuffers();
        CreateSynchronisation();
    }

    RVKSwapChain::~RVKSwapChain() {
        for (auto imageView : m_swapChainImageViews) {
            vkDestroyImageView(RVKDevice::s_rvkDevice->GetLogicalDevice(), imageView, nullptr);
        }
        m_swapChainImageViews.clear();

        if (m_swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_swapChain, nullptr);
            m_swapChain = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_depthImages.size(); ++i) {
            vkDestroyImageView(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_depthImageViews[i], nullptr);
            vkDestroyImage(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_depthImages[i], nullptr);
            vkFreeMemory(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_depthImageMemorys[i], nullptr);
        }

        for (size_t i = 0; i < m_colorImages.size(); ++i) {
            vkDestroyImageView(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_colorImageViews[i], nullptr);
            vkDestroyImage(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_colorImages[i], nullptr);
            vkFreeMemory(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_colorImageMemorys[i], nullptr);
        }

        for (auto framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(RVKDevice::s_rvkDevice->GetLogicalDevice(), framebuffer, nullptr);
        }

        vkDestroyRenderPass(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_renderPass, nullptr);

        for (u32 i = 0; i < MAX_FRAME_DRAWS; ++i) {
            vkDestroySemaphore(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_imageAvailable[i], nullptr);
            vkDestroySemaphore(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_renderFinished[i], nullptr);
            vkDestroyFence(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_drawFences[i], nullptr);
        }
        VK_CORE_INFO("SwapChain Destroyed!");
    }

    void RVKSwapChain::CreateSwapChain() {
        // Get Swap Chain details so we can pick best settings 
        SwapChainDetails swapChainDetails = RVKDevice::s_rvkDevice->GetSwapChainDetails();

        // Find optimal surface values for our swap chain
        VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainDetails.formats);
        VkPresentModeKHR presentMode = ChoosePresentMode(swapChainDetails.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainDetails.capabilities);

        // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
        // If imageCount higher than max, then clamp down to max
        // if 0, then limitless
        u32 imageCount = swapChainDetails.capabilities.minImageCount + 1;

        if (swapChainDetails.capabilities.maxImageCount < imageCount) {
            imageCount = swapChainDetails.capabilities.maxImageCount;
        }

        // Creation information for swap chain
        VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = RVKDevice::s_rvkDevice->GetSurface();														// Swapchain surface
        swapChainCreateInfo.imageFormat = surfaceFormat.format;										// Swapchain format
        swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;								// Swapchain colour space
        swapChainCreateInfo.presentMode = presentMode;												// Swapchain presentation mode
        swapChainCreateInfo.imageExtent = extent;													// Swapchain image extents
        swapChainCreateInfo.minImageCount = imageCount;												// Minimum images in swapchain
        swapChainCreateInfo.imageArrayLayers = 1;													// Number of layers for each image in chain
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// What attachment images will be used as
        swapChainCreateInfo.preTransform = swapChainDetails.capabilities.currentTransform;	        // Transform to perform on swap chain images
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// How to handle blending images with external graphics(e.g. other windows)
        swapChainCreateInfo.clipped = VK_TRUE;														// Whether to clip parts of image not in view (e.g. behind another window, off screen, etc)

        // Get Queue Family Indices
        QueueFamilyIndices indices = RVKDevice::s_rvkDevice->GetQueueFamilies();

        // If Graphics and Presentation families are different, then swapchain must let images be shared between families
        if (indices.graphicsFamily != indices.presentFamily) {
            // Queue to share between
            u32 queueFamilyIndices[] = {
                indices.graphicsFamily,
                indices.presentFamily,
            };

            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;		// Image share handling
            swapChainCreateInfo.queueFamilyIndexCount = 2;							// Numberof queues to share images between
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;			// Array of queues to share between
        }
        else {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0;
            swapChainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        // If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
        swapChainCreateInfo.oldSwapchain = m_oldSwapChain != VK_NULL_HANDLE ? m_oldSwapChain->m_swapChain : VK_NULL_HANDLE;

        // Create Swapchain
        VkResult result = vkCreateSwapchainKHR(RVKDevice::s_rvkDevice->GetLogicalDevice(), &swapChainCreateInfo, nullptr, &m_swapChain);
        VK_CHECK(result, "Failed to create a Swapchain!");

        // Store for later reference
        m_imageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;

        // Get swap chain images (first count, then values)
        u32 swapChainImageCount;
        vkGetSwapchainImagesKHR(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_swapChain, &swapChainImageCount, nullptr);
        std::vector<VkImage> images(swapChainImageCount);
        vkGetSwapchainImagesKHR(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_swapChain, &swapChainImageCount, images.data());

        for (VkImage image : images) {
            // Add to swapchain image list
            m_swapChainImages.push_back(image);
            m_swapChainImageViews.push_back(RVKDevice::s_rvkDevice->CreateImageView(image, m_imageFormat, VK_IMAGE_ASPECT_COLOR_BIT));
        }
    }

    void RVKSwapChain::CreateRenderPass() {
        // Array of our subpasses
        std::array<VkSubpassDescription, 2> subpasses{};

        // ATTACHMENTS
        // SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

        // Colour Attachment (Input)
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = m_imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth attachment (Input)
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = RVKDevice::s_rvkDevice->FindDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Colour Attachment (Input) Reference
        VkAttachmentReference colourAttachmentReference = {};
        colourAttachmentReference.attachment = 1;
        colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth Attachment (Input) Reference
        VkAttachmentReference depthAttachmentReference = {};
        depthAttachmentReference.attachment = 2;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Set up Subpass 1
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = &colourAttachmentReference;
        subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

        // SUBPASS 2 ATTACHMENTS + REFERENCES

        // Swapchain colour attachment
        VkAttachmentDescription swapChainColorAttachment = {};
        swapChainColorAttachment.format = m_imageFormat;                     // Format to use for attachment
        swapChainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
        swapChainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Describes what to do with attachment before rendering
        swapChainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachemnt after rendering
        swapChainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
        swapChainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

        // Framebuffer data will be stored as an image, but images can be given different data layouts
        // to give optimal use for certain operations
        swapChainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // Image data layout before render pass starts
        swapChainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     // Image data layout after render pass (to change to)

        // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
        VkAttachmentReference swapChainColorAttachmentReference = {};
        swapChainColorAttachmentReference.attachment = 0;
        swapChainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // References to attachment that subpass will take input from
        std::array<VkAttachmentReference, 2> inputReferences;
        inputReferences[0].attachment = 1;
        inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputReferences[1].attachment = 2;
        inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Set up Subpass 2
        subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[1].colorAttachmentCount = 1;
        subpasses[1].pColorAttachments = &swapChainColorAttachmentReference;
        subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
        subpasses[1].pInputAttachments = inputReferences.data();

        // SUBPASS DEPENDENCIES

        // Need to determine when layout transitions occur using subpass dependencies
        std::array<VkSubpassDependency, 3> subpassDependencies;

        // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        // Transition must happen after...
        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        // Subpass index(VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     // Pipeline stage
        subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               // Stage access mask(memory access)
        // But must happen before...
        subpassDependencies[0].dstSubpass = 0;
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[0].dependencyFlags = 0;

        // Subpass 1 layout (colour/ depth) to Subpass 2 layout (shader read)
        subpassDependencies[1].srcSubpass = 0;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[1].dstSubpass = 1;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        subpassDependencies[1].dependencyFlags = 0;

        // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        // Transition must happen after...
        subpassDependencies[2].srcSubpass = 0;
        subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        // But must happen before...
        subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependencies[2].dependencyFlags = 0;

        std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapChainColorAttachment, colorAttachment, depthAttachment };

        // Create info for Render Pass
        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
        renderPassCreateInfo.pAttachments = renderPassAttachments.data();
        renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassCreateInfo.pSubpasses = subpasses.data();
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderPassCreateInfo.pDependencies = subpassDependencies.data();

        VkResult result = vkCreateRenderPass(RVKDevice::s_rvkDevice->GetLogicalDevice(), &renderPassCreateInfo, nullptr, &m_renderPass);
        VK_CHECK(result, "Failed to create a Render Pass!");
    }

    void RVKSwapChain::CreateColourBufferImage() {
        // Resize supported format for colour attachment
        m_colorImages.resize(m_swapChainImages.size());
        m_colorImageMemorys.resize(m_swapChainImages.size());
        m_colorImageViews.resize(m_swapChainImages.size());

        // Get supported format for colour attachment
        VkFormat colorFormat = m_imageFormat;

        for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
            // Create Colour Buffer Image
            m_colorImages[i] = RVKDevice::s_rvkDevice->CreateImage(m_swapChainExtent.width, m_swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_colorImageMemorys[i]);

            // Create Colour Buffer Image View
            m_colorImageViews[i] = RVKDevice::s_rvkDevice->CreateImageView(m_colorImages[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void RVKSwapChain::CreateDepthBufferImage() {
        m_depthImages.resize(m_swapChainImages.size());
        m_depthImageMemorys.resize(m_swapChainImages.size());
        m_depthImageViews.resize(m_swapChainImages.size());

        // Get supported format for depth buffer
        VkFormat depthFormat = RVKDevice::s_rvkDevice->FindDepthFormat();

        for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
            // Create Depth Buffer Image
            m_depthImages[i] = RVKDevice::s_rvkDevice->CreateImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthImageMemorys[i]);

            // Create Depth Buffer Image View
            m_depthImageViews[i] = RVKDevice::s_rvkDevice->CreateImageView(m_depthImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        }
    }

    void RVKSwapChain::CreateFramebuffers() {
        // Resize framebuffer count to equal swap chain image count
        m_framebuffers.resize(m_swapChainImages.size());

        // Create a framebuffer for each swap chain image
        for (size_t i = 0; i < m_framebuffers.size(); ++i) {
            std::array<VkImageView, 3> attachments = {
                m_swapChainImageViews[i],
                m_colorImageViews[i],
                m_depthImageViews[i]
            };

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = m_renderPass;                                          // Render Pass layout the Framebuffer will be used with
            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCreateInfo.pAttachments = attachments.data();                                // List of attachments(1:1 with Render Pass)
            framebufferCreateInfo.width = m_swapChainExtent.width;                                    // Framebuffer width
            framebufferCreateInfo.height = m_swapChainExtent.height;                                  // FRamebuffer height
            framebufferCreateInfo.layers = 1;                                                       // Framebuffer layers

            VkResult result = vkCreateFramebuffer(RVKDevice::s_rvkDevice->GetLogicalDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
            VK_CHECK(result, "Failed to create a Framebuffer!");
        }
    }

    void RVKSwapChain::CreateSynchronisation() {
        m_imageAvailable.resize(MAX_FRAME_DRAWS);
        m_renderFinished.resize(MAX_FRAME_DRAWS);
        m_drawFences.resize(MAX_FRAME_DRAWS);
        m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

        // Semaphore create information
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Fence create information
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i) {
            if (vkCreateSemaphore(RVKDevice::s_rvkDevice->GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS
                || vkCreateSemaphore(RVKDevice::s_rvkDevice->GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS
                || vkCreateFence(RVKDevice::s_rvkDevice->GetLogicalDevice(), &fenceCreateInfo, nullptr, &m_drawFences[i]) != VK_SUCCESS) {
                VK_CORE_ERROR("Failed to create a Semaphore and/or Fence!");
            }
        }
    }

    VkSurfaceFormatKHR RVKSwapChain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> formats) {
        // If inly 1 format available and is undefined, then this mean ALL formats are available (no restrictions)
        if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
            return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        // If restricted, search for optimal format
        for (const auto& format : formats) {
            if ((format.format == VK_FORMAT_R8G8B8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
                && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }

        // If can't find optimal format, then just return first format
        return formats[0];
    }

    VkPresentModeKHR RVKSwapChain::ChoosePresentMode(const std::vector<VkPresentModeKHR> presentModes) {
        // Look for Mailbox presentation mode
        for (const auto& presentMode : presentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return presentMode;
            }
        }

        // If can't find, use FIFO as Vulkan spec says it must be present
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D RVKSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            // If value can vary, need to set manually

            // Get window size
            //int width, height;
            //glfwGetFramebufferSize(m_window, &width, &height);

            // Create new extent using window size
            VkExtent2D newExtent = m_windowExtent;
            // Surface also defines max and min, so make sure within boundaries by clamping value
            newExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, newExtent.width));
            newExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, newExtent.height));

            return newExtent;
        }
    }

    VkResult RVKSwapChain::AcquireNextImage(u32& imageIndex) {
        // -- GET NEXT IMAGE --
          // Wait for given fence to signal (open) from last draw before continuing
        vkWaitForFences(RVKDevice::s_rvkDevice->GetLogicalDevice(), 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<u64>::max());

        // Get index of next image to be drawn to, and signal semaphore when ready to br drawn to
        VkResult result = vkAcquireNextImageKHR(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_swapChain, std::numeric_limits<u64>::max(), m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

        return result;
    }

    VkResult RVKSwapChain::SubmitCommandBuffers(const VkCommandBuffer& commandBuffer, const u32& imageIndex) {
        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(RVKDevice::s_rvkDevice->GetLogicalDevice(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[imageIndex] = m_drawFences[m_currentFrame];

        // -- SUBMIT COMAND BUFFER TO RENDER --
        // Queue submission information
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_imageAvailable[m_currentFrame];
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_renderFinished[m_currentFrame];

        // Manually reset (close) fence
        vkResetFences(RVKDevice::s_rvkDevice->GetLogicalDevice(), 1, &m_drawFences[m_currentFrame]);
        // Submit command buffer to queue
        VkResult result = vkQueueSubmit(RVKDevice::s_rvkDevice->GetGraphicsQueue(), 1, &submitInfo, m_drawFences[m_currentFrame]);
        VK_CHECK(result, "Failed to submit Command Buffer to Queue!");

        // -- PRESENT RENDERED IMAGE TO SCREEN --
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderFinished[m_currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapChain;
        presentInfo.pImageIndices = &imageIndex;

        // Present image
        result = vkQueuePresentKHR(RVKDevice::s_rvkDevice->GetPresentQueue(), &presentInfo);

        // Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;

        return result;
    }

    bool RVKSwapChain::CompareSwapFormats(const RVKSwapChain& swapChain) const {
        return swapChain.m_imageFormat == m_imageFormat && swapChain.m_depthFormat == m_depthFormat;
    }
}