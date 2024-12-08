#include "Framework/RVKApp.h"

int main() {
	RVK::Log::Init();
	RVK::RVKApp app{};

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	RVK::Log::Shutdown();
	return EXIT_SUCCESS;
}