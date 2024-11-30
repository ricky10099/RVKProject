#pragma once
#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class RVKWindow {
	public:
		RVKWindow(u32 width, u32 height, const std::string& title);
		~RVKWindow();

		NO_COPY(RVKWindow)

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		bool ShouldClose() { glfwPollEvents(); return glfwWindowShouldClose(m_window); }
		void ResetWindowResizedFlag() { m_resized = false; }

		// Getter Functions
		VkExtent2D GetExtent() { return { static_cast<u32>(m_width), static_cast<u32>(m_height) }; }
		bool IsWindowResized() { return m_resized; }
		GLFWwindow* GetNativeWindow() { return m_window; }

	private:
		GLFWwindow* m_window;

		u32 m_width;
		u32 m_height;

		bool m_resized = false;

	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void InitWindow();
	};
}