#include "Framework/Scene.h"

#include "Framework/Entity.h"

namespace RVK {
	GameScene::GameScene() /*: m_debugCamera(CreateEntity("Debug Camera"))*/{
		Entity m_debugCamera = CreateEntity("Debug Camera");
		m_debugCamera.AddComponent<Components::Camera>(true);
		m_debugCamera.AddComponent<Components::Transform>(glm::vec3{ 0.0f, 0.0f, 2.5f });
	}

	Entity GameScene::CreateEntity(std::string_view name) {
		Entity entity(m_entityRoot.create(), this, name);
		m_entityMap[name] = entity.GetEntityID();

		return entity;
	}
}