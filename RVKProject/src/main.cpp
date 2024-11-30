#include "Framework/RVKApp.h"

int main() {
	RVK::Log::Init();
	RVK::RVKApp app(1280, 720, "VulkanApp");

	while (app.StartFrame()) {
		app.EndFrame();
	}

	RVK::Log::Shutdown();
}