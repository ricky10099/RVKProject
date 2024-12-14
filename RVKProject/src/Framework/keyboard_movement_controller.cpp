#include "Framework/keyboard_movement_controller.h"
#include "Framework/Component.h"

namespace RVK {
	void KeyboardMovementController::MoveInPlaneXZ(
		GLFWwindow* window, float dt, Entity& entity, GameObject& camera) {
		glm::vec3 rotate{ 0 };

		const glm::vec3 forwardDir{ 0.f, 0.f, -1.0f };
		const glm::vec3 rightDir{ 1, 0.f, 0 };
		const glm::vec3 upDir{ 0.f, 1.f, 0.f };

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