#pragma once

#include "Framework/Utils.h"
#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKRenderer.h"
#include "Framework/Vulkan/RVKDescriptors.h"
#include "Framework/GameObject.h"

namespace RVK {
	class RVKApp {
	 public:
	  static constexpr int WIDTH = 1280;
	  static constexpr int HEIGHT = 720;

	  RVKApp();
	  ~RVKApp();

	  NO_COPY(RVKApp)

	  void Run();

	 private:
	  void LoadGameObjects();

	  RVKWindow m_rvkWindow{WIDTH, HEIGHT, "Vulkan App"};
	  RVKRenderer m_rvkRenderer{m_rvkWindow};

	  // note: order of declarations matters
	  std::unique_ptr<RVKDescriptorPool> globalPool{};
	  GameObject::Map gameObjects;
	};
}  // namespace RVK
