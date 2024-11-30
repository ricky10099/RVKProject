#include <set>

#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
    std::shared_ptr<RVKDevice> RVKDevice::s_rvkDevice;

    RVKDevice::RVKDevice(RVKWindow* window) : m_rvkWindow(window) {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
    }

    RVKDevice::~RVKDevice() {
        if (!m_destroyed) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);

            vkDestroyDevice(m_device, nullptr);

            if (ENABLE_VALIDATION) {
                DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            }

            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            vkDestroyInstance(m_instance, nullptr);

            VK_CORE_INFO("Vulkan device destroyed");
            m_destroyed = true;
        }
    }

    void RVKDevice::CreateInstance() {
        if (ENABLE_VALIDATION && !CheckValidationLayerSupport()) {
            VK_CORE_ERROR("Validation layers requested, but not available!");
        }

        // Information about the application itself
        // Most data here doesn't affect the program and is for developer convenience
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan App";					// Custom name of the application
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom version of the application
        appInfo.pEngineName = "No Engine";							// Custom engine name
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);			// Custom engine version
        appInfo.apiVersion = VK_API_VERSION_1_3;					// The Vulkan Version

        // Createion information for a VkInstance (Vulkan Instance)
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Create list to hold instance extensions
        std::vector<const char*> instanceExtensions = std::vector<const char*>();

        // Set up extensions Instance will use
        uint32_t glfwExtensionCount = 0;		// GLFW may require multiple extensions
        const char** glfwExtensions;			// Extensions passed as array of cstrings, so need pointer (the array) to pointer(the cstring)

        // Get GLFW extensions
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Add GLFW extensions to list of extensions
        for (size_t i = 0; i < glfwExtensionCount; ++i) {
            instanceExtensions.push_back(glfwExtensions[i]);
        }

        // If validation enabled, add extension to report validation debug info
        if (ENABLE_VALIDATION) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Check Instance Extensions supported...
        if (!CheckInstanceExtensionSupport(&instanceExtensions)) {
            VK_CORE_ERROR("VkInstance does not support required extensions!");
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        if (ENABLE_VALIDATION) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.pNext = nullptr;
        }

        // Create instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        VK_CHECK(result, "Failed to create a Vulkan Instance!");
    }

    void RVKDevice::SetupDebugMessenger() {
        if (!ENABLE_VALIDATION) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
        VK_CHECK(result, "Failed to set up debug messenger!");
    }

    void RVKDevice::CreateSurface() {
        m_rvkWindow->CreateWindowSurface(m_instance, &m_surface);
    }

    void RVKDevice::PickPhysicalDevice() {
        // Enumerate Physical devices the vkInstance can access
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        // If no device available, then none support Vulkan!
        if (deviceCount == 0) {
            VK_CORE_ERROR("Can't find GPUs that support Vulkan Instance!");
        }

        std::vector<VkPhysicalDevice> deviceList(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, deviceList.data());

        for (const auto& device : deviceList) {
            if (CheckDeviceSuitable(device)) {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        // Information about the device itself (ID, name, type, vendor, etc)
        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
    }

    void RVKDevice::CreateLogicalDevice() {
        // Vector for queue creation information, and set for family indices
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<u32> queueFamilyIndices = { m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.presentFamily };

        // Queue the logical device needs to create and info to do so(Only 1 for now, will add more later!)
        float priority = 1.0f;
        for (u32 queueFamilyIndex : queueFamilyIndices) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;					// The index of the family to create a queue from
            queueCreateInfo.queueCount = 1;											// Number of queues to create             
            queueCreateInfo.pQueuePriorities = &priority;							// Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)

            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Physical Device Features the Logical Device will be using
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;     // Enable Anisotropy

        // Information to create logical device (sometimes called "device")
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		// Number if Queue Create Infos
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								// List of queue create infos so device can create required queues
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());	// Number of enabled logical device extensions
        deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();						// List of enabled logical device extensions
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;			// Physical Device features Logical Device will use

        // Create the logical device for the given physical devic
        VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        VK_CHECK(result, "Failed to create a Logical Device!");

        // Queues are created at the same time as the device...
        // So we want handle to queues
        // From given logical device, of given Queue Family, of given Queue Index(0 since only one queue), place reference in given VkQueue
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
    }

    void RVKDevice::CreateCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;      // Queue Family type that buffers from this command pool will use

        // Create a Graphics queue Family Command Pool
        VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        VK_CHECK(result, "Failed to create a Command Pool!");
    }

    bool RVKDevice::CheckValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : VALIDATION_LAYERS) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    bool RVKDevice::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions) {
        // Need to get number of extensions to create array of correct size to hold extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        // Create a list of VkExtensionProperties using count
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        // Check if given extensions are in list of available extensions
        for (const auto& checkExtension : *checkExtensions) {
            bool hasExtension = false;
            for (const auto& extension : extensions) {
                if (strcmp(checkExtension, extension.extensionName)) {
                    hasExtension = true;
                    break;
                }
            }

            if (!hasExtension) {
                return false;
            }
        }

        return true;
    }

    bool RVKDevice::CheckDeviceSuitable(VkPhysicalDevice device) {
        // Information about what the device can do (geo shader, tess shader, wide line, etc)
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        m_queueFamilyIndices = FindQueueFamilies(device);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapChainValid = false;
        if (extensionsSupported) {
            m_swapChainDetails = FindSwapChainDetails(device);
            swapChainValid = !m_swapChainDetails.presentModes.empty() && !m_swapChainDetails.formats.empty();
        }

        return m_queueFamilyIndices.IsValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
    }

    QueueFamilyIndices RVKDevice::FindQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

        // Go through each queue family and check if it has at least 1 of the required types of queue
        int i = 0;
        for (const auto& queueFamily : queueFamilyList) {
            // First check if queue family has at least 1 queue in that family (could have no queues)
            // Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;		// If queue family is valid, then get index
            }

            // Check if Queue Family supports prestation
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.presentFamily = i;
            }

            // Check if queue family indices are in a valid state, stop searching if so
            if (indices.IsValid()) {
                break;
            }

            ++i;
        }

        return indices;
    }

    bool RVKDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
        // Get device extension count;
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        // If no extensions found, return false
        if (extensionCount == 0) {
            return false;
        }

        // Populate list of extensions
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

        // Check for extension
        for (const auto& deviceExtension : DEVICE_EXTENSIONS) {
            bool hasExtension = false;
            for (const auto& extension : extensions) {
                if (strcmp(deviceExtension, extension.extensionName) == 0) {
                    hasExtension = true;
                    break;
                }
            }

            if (!hasExtension) {
                return false;
            }
        }

        return true;
    }

    SwapChainDetails RVKDevice::FindSwapChainDetails(VkPhysicalDevice device) {
        SwapChainDetails swapChainDetails;

        // -- CAPABILITIES --
        // Get the surface capabilities for the given surface on the given physical device
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.capabilities);

        // -- FORMATS --
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        // If formats returned, get list of formats
        if (formatCount != 0) {
            swapChainDetails.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, swapChainDetails.formats.data());
        }

        // -- PRESENTATION MODES --
        uint32_t presentationCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, nullptr);

        // If presentation modes returned, get list of presentation modes
        if (presentationCount != 0) {
            swapChainDetails.presentModes.resize(presentationCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, swapChainDetails.presentModes.data());
        }

        return swapChainDetails;
    }

    VkFormat RVKDevice::FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
        // Loop thorugh options and find compatible one
        for (VkFormat format : formats) {
            // Get properties for give format on this device
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);

            // Depending on tiling choice, need to check for different bit flag
            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        VK_CORE_ERROR("Failed to find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat RVKDevice::FindDepthFormat() {
        return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkImage RVKDevice::CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory) {
        // CREATE IMAGE
        // Image Creation Info
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                   // Type of image (1D, 2D, or 3D)
        imageCreateInfo.extent.width = width;                           // Width of image extent
        imageCreateInfo.extent.height = height;                         // Height of image extent
        imageCreateInfo.extent.depth = 1;                               // Depth of image (just 1, no 3D aspect)
        imageCreateInfo.mipLevels = 1;                                  // Number of mipmap levels
        imageCreateInfo.arrayLayers = 1;                                // Number of levels in image array
        imageCreateInfo.format = format;                                // Format type of image
        imageCreateInfo.tiling = tiling;                                // How many data should be "tiled" (arranged for optimal reading)
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Layout of image data on creation
        imageCreateInfo.usage = useFlags;                               // Bit flags defining what image will be used for
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                // Number of samples for multi-sampling
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;        // Whether image can be shared between queues

        // Create image
        VkImage image;
        VkResult result = vkCreateImage(m_device, &imageCreateInfo, nullptr, &image);
        VK_CHECK(result, "Failed to create an Image!");

        // CREATE MEMORY FOR IMAGE
        // Get memory requirements for a type of image
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(m_device, image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocInfo = {};
        memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocInfo.allocationSize = memoryRequirements.size;
        memoryAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, propFlags);

        result = vkAllocateMemory(m_device, &memoryAllocInfo, nullptr, imageMemory);
        VK_CHECK(result, "Failed to allocate memory for image!");

        // Connect memory to image
        vkBindImageMemory(m_device, image, *imageMemory, 0);

        return image;
    }

    VkImageView RVKDevice::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = image;											// Image to create view for
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;						// Type of image(1D, 2D, 3D, Cube, etc)
        viewCreateInfo.format = format;											// Format of image data
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Allows remapping of rgba components to other rgba values
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Subresources allow the view to view only a part of an image
        viewCreateInfo.subresourceRange.aspectMask = aspectFlags;				// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
        viewCreateInfo.subresourceRange.baseMipLevel = 0;						// Start mipmap level to view from
        viewCreateInfo.subresourceRange.levelCount = 1;							// Number of mipmap levels to view
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;						// Start array level to view from
        viewCreateInfo.subresourceRange.layerCount = 1;							// Number of array levels to view

        // Create image view and return it
        VkImageView imageView;
        VkResult result = vkCreateImageView(m_device, &viewCreateInfo, nullptr, &imageView);
        VK_CHECK(result, "Failed to create an Image View!");

        return imageView;
    }

    u32 RVKDevice::FindMemoryType(u32 allowedTypes, VkMemoryPropertyFlags properties) {
        // Get properties of physical device memory
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((allowedTypes & (1 << i))															// index of memory type must match corresponding bit is allowedTypes
                && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {	// Desired property bit flags are part of memory type's property flags
                // This memory type is valid, so return index
                return i;
            }
        }

        VK_CORE_CRITICAL("Failed to find suitable Memory type!");
        return 0;
    }

    void RVKDevice::CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags,
        VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
        // CREATE VERTEX BUFFER
        // Information to create a buffer(doesn't include assigning memory)
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;		                        // Size of buffer (size of 1 vertex * number of vertices)
        bufferInfo.usage = bufferUsageFlags;		                // Multiple types of buffer possible
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

        VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, buffer);
        VK_CHECK(result, "Failed to create Vertex Buffer!");

        // GET BUFFER MEMORY REQUIREMENTS
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, *buffer, &memRequirements);

        // ALLOCATE MEMORY TO BUFFER
        VkMemoryAllocateInfo memoryAllocInfo = {};
        memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocInfo.allocationSize = memRequirements.size;
        memoryAllocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,	// Index of memory type on Physical Device that has required bit flags
            bufferProperties);	            // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allow placement of data straight into buffer after mapping(otherwise would have to specify manually)
// Allocate memory to VkDeviceMemory
        result = vkAllocateMemory(m_device, &memoryAllocInfo, nullptr, bufferMemory);
        VK_CHECK(result, "Failed to allocate Vertex Buffer Memory!");

        // Allocate memory to given vertex buffer
        vkBindBufferMemory(m_device, *buffer, *bufferMemory, 0);
    }

    VkCommandBuffer RVKDevice::BeginCommandBuffer() {
        // Command buffer to hold transfer commands
        VkCommandBuffer commandBuffer;

        // Command Buffer details
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        // Allocate command buffer from pool
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

        // Information to begin command buffer record
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  // we're only using the command buffer once, so set up for one time submit

        // Begin recording transfer commands
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void RVKDevice::EndCommandBuffer(VkCommandBuffer& commandBuffer) {
        // End commands
        vkEndCommandBuffer(commandBuffer);

        // Queue submission information
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Submit transfer command to transfer queue and wait until finishes
        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        // Free temporary command buffer back to pool
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

    void RVKDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize) {
        // Create buffer
        VkCommandBuffer transferCommandBuffer = BeginCommandBuffer();

        // Region of data to copy from and to
        VkBufferCopy bufferCopyRegion = {};
        bufferCopyRegion.srcOffset = 0;
        bufferCopyRegion.dstOffset = 0;
        bufferCopyRegion.size = bufferSize;

        // Command to copy src buffer to dst buffer
        vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

        EndCommandBuffer(transferCommandBuffer);
    }

    void RVKDevice::CopyImageBuffer(VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height) {
        // Create buffer
        VkCommandBuffer transferCommandBuffer = BeginCommandBuffer();

        VkBufferImageCopy imageRegion = {};
        imageRegion.bufferOffset = 0;                                           // Offset into data
        imageRegion.bufferRowLength = 0;                                        // Row length of data to calculate data spacing
        imageRegion.bufferImageHeight = 0;                                      // Image height to calculate data spacing
        imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Which aspect of image to copy
        imageRegion.imageSubresource.mipLevel = 0;                              // Mipmap level to copy
        imageRegion.imageSubresource.baseArrayLayer = 0;                        // Starting array layer (if array)
        imageRegion.imageSubresource.layerCount = 1;                            // Number of layers to copy starting at baseArrayLayer
        imageRegion.imageOffset = { 0, 0, 0 };                                  // Offset into image (as opposed to raw data in bufferOffset)
        imageRegion.imageExtent = { width, height, 1 };                         // Size of region to copy as(x, y, z) values

        // Copy buffer to given image
        vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

        EndCommandBuffer(transferCommandBuffer);
    }

    void RVKDevice::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
        // Create buffer
        VkCommandBuffer commandBuffer = BeginCommandBuffer();

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = oldLayout;                                    // Layout to transition from
        imageMemoryBarrier.newLayout = newLayout;                                    // Layout to transition to
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;            // Queue family to transition from
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;            // Queue family to transition to
        imageMemoryBarrier.image = image;                                            // Image being accessed and modified as part of barrier
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Aspect of image being altered
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;                        // First mip level to start alterations on
        imageMemoryBarrier.subresourceRange.levelCount = 1;                          // Number of mip levels to alter starting from baseMipLevel 
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;                      // First layer to start alterations on
        imageMemoryBarrier.subresourceRange.layerCount = 1;                          // Number of layers to alter stating from baseArrayLayer

        VkPipelineStageFlags srcStage{};
        VkPipelineStageFlags dstStage{};

        // If transitioning from new image to image ready to receive data...
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            imageMemoryBarrier.srcAccessMask = 0;                                    // Memory access stage transition must after...
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;         // Memory access stage transition must before...

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        // If fransitioning from transfer destination to shader readable...
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,     // Pipeline stages(match to src and dst AccessMasks)
            0,                      // Dependency flags
            0, nullptr,             // Memory Barrier count + data
            0, nullptr,             // Buffer Memory Barrier count + data
            1, &imageMemoryBarrier  // Image Memory Barrier count + data
        );

        EndCommandBuffer(commandBuffer);
    }
}