#include "Framework/Entity.h"

namespace RVK {
	Entity::Entity(entt::entity entity, std::string_view name, const Scene* scene) : m_entity(entity), m_name(name), m_scene(scene){}

	void Entity::Move(const glm::vec3& translation) {
		m_position += translation;
	}

	void Entity::MoveTo(const glm::vec3& position) {
		m_position = position;
	}
}