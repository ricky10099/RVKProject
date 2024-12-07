#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/VKValidate.h"

namespace RVK {
	std::shared_ptr<RVKDevice> RVKDevice::s_rvkDevice;

	// class member functions
	RVKDevice::RVKDevice(RVKWindow& window) : m_rvkWindow{ window } {
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
	}

	RVKDevice::~RVKDevice() {
		vkDestroyCommandPool(m_device, m_commandPool, nullptr);
		vkDestroyDevice(m_device, nullptr);

		if (ENABLE_VALIDATION) {
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	void RVKDevice::CreateInstance() {
		if (ENABLE_VALIDATION && !CheckValidationLayerSupport()) {
			VK_CORE_CRITICAL("Validation Layers requested, but not available!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "RVKApp";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (ENABLE_VALIDATION) {
			createInfo.enabledLayerCount = static_cast<u32>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
		VK_CHECK(result, "Failed to Create a Instance!")

		HasGflwRequiredInstanceExtensions();
	}

	void RVKDevice::PickPhysicalDevice() {
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			VK_CORE_CRITICAL("Failed to Find GPUs with Vulkan support!");
		}
		VK_CORE_INFO("Device count: {0}", deviceCount);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (IsDeviceSuitable(device)) {
				m_physicalDevice = device;
				break;
			}
		}

		if (m_physicalDevice == VK_NULL_HANDLE) {
			VK_CORE_CRITICAL("Failed to Find a suitable GPU!");
		}

		vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
		VK_CORE_INFO("Physical Device: ", m_properties.deviceName);
	}

	void RVKDevice::CreateLogicalDevice() {
		QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<u32> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for (u32 queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<u32>(DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

		VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
		VK_CHECK(result, "Failed to Create a Logical Device!")

		vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
	}

	void RVKDevice::CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
		VK_CHECK(result, "Failed to Create Command Pool!");
	}

	void RVKDevice::CreateSurface() { m_rvkWindow.CreateWindowSurface(m_instance, &m_surface); }

	bool RVKDevice::IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
	}

	void RVKDevice::SetupDebugMessenger() {
		if (!ENABLE_VALIDATION) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);
		VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
		VK_CHECK(result, "Failed to Set Up Debug Messenger!")
	}

	bool RVKDevice::CheckValidationLayerSupport() {
		u32 layerCount;
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

	std::vector<const char*> RVKDevice::GetRequiredExtensions() {
		u32 glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (ENABLE_VALIDATION) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void RVKDevice::HasGflwRequiredInstanceExtensions() {
		u32 extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		VK_CORE_INFO("Available Extensions:");
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) {
			VK_CORE_INFO("\t{0}", extension.extensionName);
			available.insert(extension.extensionName);
		}

		VK_CORE_INFO("Required Extensions:");
		auto requiredExtensions = GetRequiredExtensions();
		for (const auto& required : requiredExtensions) {
			VK_CORE_INFO("\t{0}", required);
			if (available.find(required) == available.end()) {
				VK_CORE_CRITICAL("Missing required glfw extension");
			}
		}
	}

	bool RVKDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		u32 extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			availableExtensions.data());

		std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices RVKDevice::FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				indices.graphicsFamilyHasValue = true;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
				indices.presentFamilyHasValue = true;
			}
			if (indices.IsComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails RVKDevice::QuerySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
		}

		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				m_surface,
				&presentModeCount,
				details.presentModes.data());
		}
		return details;
	}

	VkFormat RVKDevice::FindSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (
				tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		VK_CORE_CRITICAL("Failed to Find Supported Format!");
		return {};
	}

	u32 RVKDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
		for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		VK_CORE_CRITICAL("Failed to Find Suitable Memory Type!");
	}

	void RVKDevice::CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer);
		VK_CHECK(result, "Failed to Create Vertex Buffer!");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		result = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);
		VK_CHECK(result, "Failed to Allocate Vertex Buffer Memory!");

		vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}

	VkCommandBuffer RVKDevice::BeginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void RVKDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphicsQueue);

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}

	void RVKDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;  // Optional
		copyRegion.dstOffset = 0;  // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer);
	}

	void RVKDevice::CopyBufferToImage(
		VkBuffer buffer, VkImage image, u32 width, u32 height, u32 layerCount) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);

		EndSingleTimeCommands(commandBuffer);
	}

	void RVKDevice::CreateImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory) {
		VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &image);
		VK_CHECK(result, "Failed to Create Image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		result = vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory);
		VK_CHECK(result, "Failed to Allocate Image Memory!");

		result = vkBindImageMemory(m_device, image, imageMemory, 0);
		VK_CHECK(result, "Failed to Bind Image Memory!");
	}
}  // namespace RVK