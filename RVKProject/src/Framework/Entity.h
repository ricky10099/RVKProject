#pragma once

#include "Framework/Scene.h"

namespace RVK {
	class Entity {
	public:
		Entity() = default;
		Entity(entt::entity entity, GameScene* scene, std::string_view name = "Empty Entity");
		Entity(const Entity& other) = default;

		void Move(const glm::vec3& translation);
		void MoveTo(const glm::vec3& position);

		entt::entity GetEntityID() { return m_entity; }

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args){
			return m_scene->m_entityRoot.emplace<T>(m_entity, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent(){
			return m_scene->m_entityRoot.get<T>(m_entity);
		}

		template<typename T>
		void RemoveComponent(){
			return m_scene->m_entityRoot.remove<T>(m_entity);
		}

		template<typename T>
		bool HasComponent(){
			return m_scene->m_entityRoot.all_of<T>(m_entity);
		}

	private:
		entt::entity m_entity{ entt::null };
		GameScene* m_scene = nullptr;
		//const Entity* m_parent;
		std::vector<std::shared_ptr<Entity>> m_children;
		std::string_view m_name;
		glm::vec3 m_position{0.f, 0.f, 0.f};
		bool m_isVisible = true;
	};
 }