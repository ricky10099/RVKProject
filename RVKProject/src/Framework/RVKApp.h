#pragma once

#include <PxPhysicsAPI.h>

#include "Framework/Utils.h"
#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKRenderer.h"
#include "Framework/Vulkan/RVKDescriptors.h"
#include "Framework/Scene.h"
#include "Framework/Entity.h"

namespace RVK {
	class RVKApp {
	 public:
	  static constexpr int WIDTH = 1280;
	  static constexpr int HEIGHT = 720;
	  // note: order of declarations matters
	  std::unique_ptr<RVKDescriptorPool> globalPool{};

	public:
	  RVKApp();
	  ~RVKApp();

	  NO_COPY(RVKApp)

	  void Run();
	  void LoadScene(std::unique_ptr<Scene> scene);

	 private:
	  void LoadGameObjects();

	  RVKWindow m_rvkWindow{WIDTH, HEIGHT, "Vulkan App"};
	  RVKRenderer m_rvkRenderer{m_rvkWindow};

	  std::unique_ptr<Scene> m_currentScene;

	  Entity m_test;
	  Entity m_test2;
	  Entity m_testLight;
	  Entity m_testFloor;

	  ////////////////////////////////////////////////////////////
	  physx::PxDefaultAllocator m_defaultAllocator;
	  physx::PxDefaultErrorCallback m_defaultErrorCallback;
	  physx::PxFoundation* m_pFoundation = nullptr;
	  physx::PxPhysics* m_pPhysics = nullptr;
	  physx::PxDefaultCpuDispatcher* m_pDispatcher = nullptr;
	  physx::PxScene* m_pScene = nullptr;
	  physx::PxPvd* m_pPvd = nullptr;
	  physx::PxMaterial* m_pMaterial = nullptr;
	  physx::PxRigidDynamic* m_pBody = nullptr;
	  physx::PxRigidStatic* m_pFloor = nullptr;
	  ///////////////////////////////////////////////////////////////
	};

	RVKApp& GetApp();
}  // namespace RVK
