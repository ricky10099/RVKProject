#include "Framework/Scene.h"

#include "Framework/Entity.h"

namespace RVK {
	Entity Scene::CreateEntity(std::string_view name) {
		Entity entity(m_entityRoot.create(), name, this);
		return entity;
	}
}