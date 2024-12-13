#pragma once

#include <EnTT/entt.hpp>
//#include <PxPhysicsAPI.h>

#include "Framework/Camera.h"
#include "Framework/Component.h"

namespace RVK {
	class Entity;
	class GameScene {
	public:
		GameScene();
		~GameScene() = default;

		//virtual void Init();
		//virtual void PreUpdate();
		//virtual void Update([[maybe_unused]] float deltaTime);

		Entity CreateEntity(std::string_view name);

		bool IsRunning() { return m_isRunning; }
		bool IsPause() { return m_isPaused; }

		entt::registry m_entityRoot;

	private:
		//Entity m_debugCamera;
		//std::unique_ptr<physx::PxScene> m_pxScene;
		std::unordered_map<std::string_view, entt::entity> m_entityMap;

		bool m_isRunning = false;
		bool m_isPaused = false;
	};
}