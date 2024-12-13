#include "Framework/keyboard_movement_controller.h"
#include "Framework/Component.h"

namespace RVK {
	void KeyboardMovementController::MoveInPlaneXZ(
		GLFWwindow* window, float dt, Entity& entity, GameObject& camera) {
		glm::vec3 rotate{ 0 };

		const glm::vec3 forwardDir{ 0.f, 0.f, 1.0f };
		const glm::vec3 rightDir{ 1, 0.f, 0 };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };
		{
			glm::vec3 moveDir{ 0.f };

			if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) moveDir += forwardDir;
			if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) moveDir -= forwardDir;
			if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) moveDir -= rightDir;
			if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) moveDir += rightDir;

			//if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			//	gameObject.m_transform.rotation += lookSpeed * dt * glm::normalize(rotate);
			//}
			if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
				camera.m_transform.translation += moveSpeed * dt * glm::normalize(moveDir);
			}
		}
		// limit pitch values between about +/- 85ish degrees
		//gameObject.m_transform.rotation.x = glm::clamp(gameObject.m_transform.rotation.x, -1.5f, 1.5f);
		//gameObject.m_transform.rotation.y = glm::mod(gameObject.m_transform.rotation.y, glm::two_pi<float>());

		//float yaw = gameObject.m_transform.rotation.y * 0;
		//const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		//const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		//const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 moveDir{ 0.f };
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) entity.GetComponent<Components::Transform>().scale -= glm::vec3(1.f, 1.f, 1.1f) * dt;


		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
			entity.GetComponent<Components::Transform>().Translate(moveSpeed * dt * glm::normalize(moveDir));
		}
	}
}  // namespace RVK