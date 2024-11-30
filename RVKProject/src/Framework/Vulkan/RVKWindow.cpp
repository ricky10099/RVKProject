#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
	RVKWindow::RVKWindow(u32 width, u32 height, const std::string& title) {
		// Initialize GLFW 
		glfwInit();

		// Set GLFW to Not work with OpenGL
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		VK_CORE_INFO("VulkanWindow init");

		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);

		RVKDevice::s_rvkDevice = std::make_shared<RVKDevice>(this);
	}

	RVKWindow::~RVKWindow() {
		glfwDestroyWindow(m_window);
		glfwTerminate();
		VK_CORE_INFO("VulkanWindow destroy");
	}

	void RVKWindow::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, surface);
		VK_CHECK(result, "Failed to Create a Window Surface!");
	}

	void RVKWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto rvkWindow = reinterpret_cast<RVKWindow*>(glfwGetWindowUserPointer(window));
		rvkWindow->m_resized = true;
		rvkWindow->m_width = width;
		rvkWindow->m_height = height;
	}
}