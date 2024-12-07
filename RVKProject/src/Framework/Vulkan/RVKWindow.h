#pragma once

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class RVKWindow {
	public:
		RVKWindow(int width, int height, std::string name);
		~RVKWindow();

		NO_COPY(RVKWindow)

		bool ShouldClose() { return glfwWindowShouldClose(m_window); }
		VkExtent2D GetExtent() { return {static_cast<u32>(m_width), static_cast<u32>(m_height)}; }
		bool WasWindowResized() { return m_framebufferResized; }
		void ResetWindowResizedFlag() { m_framebufferResized = false; }
		GLFWwindow *GetGLFWwindow() const { return m_window; }

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

		private:
		static void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
		void InitWindow();

		int m_width;
		int m_height;
		bool m_framebufferResized = false;

		std::string m_windowName;
		GLFWwindow* m_window;
	};
}  // namespace RVK
