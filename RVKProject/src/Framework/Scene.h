#pragma once

#include <EnTT/entt.hpp>
#include <PxPhysicsAPI.h>

#include "Framework/Camera.h"

namespace RVK {
	class Entity;
	class Scene {
	public:
		Scene();
		~Scene() = default;

		virtual void Init() = 0;
		virtual void PreUpdate() = 0;
		virtual void Update([[maybe_unused]] float deltaTime) = 0;

		Entity CreateEntity(std::string_view name);

		bool IsRunning() { return m_isRunning; }
		bool IsPause() { return m_isPaused; }

	private:
		std::shared_ptr<Camera> m_mainCamera;
		std::unique_ptr<physx::PxScene> m_pxScene;
		entt::registry m_entityRoot;
		std::unordered_map<std::string_view, entt::entity> m_entityMap;

		bool m_isRunning = false;
		bool m_isPaused = false;
	};
}