#pragma once

#include "Framework/Utils.h"
#include "Framework/Vulkan/VulkanWindow.h"
#include "Framework/Vulkan/VulkanRenderer.h"


namespace RVK {
	class VulkanApp {
	public:
		VulkanApp(u32 width = 800, u32 height = 600, const std::string& title = "VkLib");
		~VulkanApp();

		void Run();

	private:
		RVKWindow m_vulkanWindow;
		RVKRenderer m_vulkanRenderer;

		//static std::vector<Model> m_modelList;
	};
}