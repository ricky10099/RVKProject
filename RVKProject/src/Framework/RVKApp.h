#pragma once

#include "Framework/Utils.h"
#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKRenderer.h"


namespace RVK {
	class RVKApp {
	public:
		RVKApp(u32 width = 800, u32 height = 600, const std::string& title = "VkLib");
		~RVKApp();

		bool StartFrame();
		void EndFrame();

	private:
		RVKWindow m_rvkWindow;
		RVKRenderer m_rvkRenderer;
	};
}