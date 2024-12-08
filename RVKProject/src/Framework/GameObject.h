#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "Framework/Model.h"

namespace RVK {
	struct TransformComponent {
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation{};

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 Mat4();

		glm::mat3 NormalMatrix();
	};

	struct PointLightComponent {
		float lightIntensity = 1.0f;
	};

	class GameObject {
	public:
		using Map = std::unordered_map<u32, GameObject>;
		glm::vec3 m_color{};
		TransformComponent m_transform{};

		// Optional pointer components
		std::shared_ptr<Model> m_model{};
		std::unique_ptr<PointLightComponent> m_pointLight = nullptr;

	public:
		static GameObject CreateGameObject() {
			static u32 currentId = 0;
			return GameObject{ currentId++ };
		}

		static GameObject MakePointLight(
			float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

		NO_COPY(GameObject)
		CAN_MOVE(GameObject)

		u32 GetId() { return m_id; }		

	private:
		u32 m_id;

	private:
		GameObject(u32 objId) : m_id{ objId } {}
	};
}  // namespace RVK
