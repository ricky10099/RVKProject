#include "Framework/Entity.h"

namespace RVK {
	Entity::Entity(entt::entity entity, Scene* scene, std::string_view name) : m_entity(entity), m_scene(scene), m_name(name){}

	void Entity::Move(const glm::vec3& translation) {
		m_position += translation;
	}

	void Entity::MoveTo(const glm::vec3& position) {
		m_position = position;
	}
}