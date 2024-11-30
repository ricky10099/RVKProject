#include "Framework/VulkanApp.h"

int main() {
	RVK::Log::Init();
	RVK::VulkanApp app(1280, 720, "VulkanApp");

	app.Run();

	RVK::Log::Shutdown();
}