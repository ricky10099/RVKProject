#pragma once

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class SceneCamera {
	public:
		SceneCamera();
		~SceneCamera() = default;

		void SetOrthographicProjection(
			float left, float right, float top, float bottom, float near, float far);
		void SetPerspectiveProjection(float fovy, float aspect, float near, float far);
		void SetViewDirection(
			glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, 1.f, 0.f });
		void SetViewTarget(
			glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.f, 1.f, 0.f });
		void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);

		const glm::mat4& GetProjection() const { return m_projectionMatrix; }
		const glm::mat4& GetView() const { return m_viewMatrix; }
		//const glm::mat4& GetInverseView() const { return m_inverseViewMatrix; }
		const glm::vec3 GetPosition() const { return glm::vec3(m_viewMatrix[3]); }

	private:
		glm::mat4 m_projectionMatrix{ 1.f };
		glm::mat4 m_viewMatrix{ 1.f };
		//glm::mat4 m_inverseViewMatrix{ 1.f };

		float m_aspect = 1280.0f / 720.0f;
	};
}  // namespace RVK