#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKDevice.h"
namespace RVK {
	RVKWindow::RVKWindow(int width, int height, std::string name) : m_width{ width }, m_height{ height }, m_windowName{ name } {
		InitWindow();
	}

	RVKWindow::~RVKWindow() {
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void RVKWindow::InitWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_window = glfwCreateWindow(m_width, m_height, m_windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);

		RVKDevice::s_rvkDevice = std::make_shared<RVKDevice>(this);
	}

	void RVKWindow::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, surface);
		VK_CHECK(result, "Failed to Craete a Window Surface!");
	}

	void RVKWindow::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto rvkWindow = reinterpret_cast<RVKWindow*>(glfwGetWindowUserPointer(window));
		rvkWindow->m_framebufferResized = true;
		rvkWindow->m_width = width;
		rvkWindow->m_height = height;
	}
}  // namespace RVK
