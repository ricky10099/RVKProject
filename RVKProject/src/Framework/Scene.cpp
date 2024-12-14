#include "Framework/Scene.h"

#include "Framework/Entity.h"

namespace RVK {
	Scene::Scene(){
		Entity m_debugCamera = CreateEntity("Debug Camera");
		m_debugCamera.AddComponent<Components::Camera>(true);
		m_debugCamera.AddComponent<Components::Transform>(glm::vec3{ 0.0f, 0.0f, 2.5f });
	}

	Entity& Scene::CreateEntity(std::string_view name) {
		Entity entity(m_entityRoot.create(), this, name);
		m_entityMap[name] = entity.GetEntityID();

		return entity;
	}
}