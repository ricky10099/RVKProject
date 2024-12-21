#pragma once

#include <PxPhysicsAPI.h>

#include <cri_le_xpt.h>
#include <cri_le_atom_ex.h>
#include <cri_le_atom_wasapi.h>

#include "Framework/Utils.h"
#include "Framework/Vulkan/RVKWindow.h"
#include "Framework/Vulkan/RVKRenderer.h"
#include "Framework/Vulkan/RVKDescriptors.h"
#include "Framework/Timestep.h"
#include "Framework/Scene.h"
#include "Framework/Entity.h"

namespace RVK {
	class RVKApp {
	 public:
	  static constexpr int WIDTH = 1280;
	  static constexpr int HEIGHT = 720;
	  // note: order of declarations matters
	  std::unique_ptr<RVKDescriptorPool> globalPool{};
	  static RVKApp* appInstance;

	public:
	  RVKApp();
	  ~RVKApp();

	  NO_COPY(RVKApp)

	  void Run();
	  void LoadScene(std::unique_ptr<Scene> scene);

	  float GetTimestep() { return m_timestep; }

	 private:
	  void LoadGameObjects();

	  RVKWindow m_rvkWindow{WIDTH, HEIGHT, "Vulkan App"};
	  RVKRenderer m_rvkRenderer{m_rvkWindow};

	  Timestep m_timestep{0ms};
	  std::chrono::steady_clock::time_point m_timeLastFrame;
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
	  CriAtomExPlayerHn m_BGMplayer;
	  CriAtomExPlayerHn m_SEplayer;
	  CriAtomExVoicePoolHn m_voicePool;
	  CriAtomExConfig m_atomexConfig;
	  CriAtomExAdxVoicePoolConfig m_adxVoicePoolpconfig;
	  CriAtomExAcbHn bgm_acb_hn;
	  CriAtomExAcbHn se_acb_hn;
	  CriAtomDbasId	m_dbasID;
	  CriAtomExPlaybackId m_playbackID;
	  CriSint32 m_cueIndex;
	  ///////////////////////////////////////////////////////////////

	};

	RVKApp& GetApp();
}  // namespace RVK
