#include "Framework/Scene.h"

#include "Framework/Entity.h"

namespace RVK {
	GameScene::GameScene() /*: m_debugCamera(CreateEntity("Debug Camera"))*/{
		Entity m_debugCamera = CreateEntity("Debug Camera");
		m_debugCamera.AddComponent<Components::Camera>().currentCamera = true;
		m_debugCamera.AddComponent<Components::Transform>();
	}

	Entity GameScene::CreateEntity(std::string_view name) {
		Entity entity(m_entityRoot.create(), this, name);
		m_entityMap[name] = entity.GetEntityID();

		return entity;
	}
}