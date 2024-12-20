#include "Framework/Camera.h"

namespace RVK {
	SceneCamera::SceneCamera() {
		SetPerspectiveProjection(glm::radians(50.f), m_aspect, 0.1f, 100.f);
	}

	void SceneCamera::SetOrthographicProjection(
		float left, float right, float top, float bottom, float zNear, float zFar) {
		m_projectionMatrix = glm::mat4{ 1.0f };
		m_projectionMatrix[0][0] = 2.f / (right - left);
		m_projectionMatrix[1][1] = 2.f / (bottom - top);
		m_projectionMatrix[2][2] = 1.f / (zFar - zNear);
		m_projectionMatrix[3][0] = -(right + left) / (right - left);
		m_projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		m_projectionMatrix[3][2] = -zNear / (zFar - zNear);
	}

	void SceneCamera::SetPerspectiveProjection(float fovy, float aspect, float zNear, float zFar) {
		VK_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		//const float tanHalfFovy = tan(fovy / 2.f);
		//m_projectionMatrix = glm::mat4{ 0.0f };
		//m_projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
		//m_projectionMatrix[1][1] = 1.f / (tanHalfFovy);
		//m_projectionMatrix[2][2] = far / (far - near);
		//m_projectionMatrix[2][3] = 1.f;
		//m_projectionMatrix[3][2] = -(far * near) / (far - near);

		m_projectionMatrix = glm::perspective(fovy, aspect, zNear, zFar);
	}

	//void SceneCamera::SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
	//	const glm::vec3 w{ glm::normalize(direction) };
	//	const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
	//	const glm::vec3 v{ glm::cross(w, u) };

	//	/*m_viewMatrix = glm::mat4{ 1.f };
	//	m_viewMatrix[0][0] = u.x;
	//	m_viewMatrix[1][0] = u.y;
	//	m_viewMatrix[2][0] = u.z;
	//	m_viewMatrix[0][1] = v.x;
	//	m_viewMatrix[1][1] = v.y;
	//	m_viewMatrix[2][1] = v.z;
	//	m_viewMatrix[0][2] = w.x;
	//	m_viewMatrix[1][2] = w.y;
	//	m_viewMatrix[2][2] = w.z;
	//	m_viewMatrix[3][0] = -glm::dot(u, position);
	//	m_viewMatrix[3][1] = -glm::dot(v, position);
	//	m_viewMatrix[3][2] = -glm::dot(w, position);*/

	//	m_viewMatrix = glm::lookAtLH(position, direction, up);

	//	//m_inverseViewMatrix = glm::mat4{ 1.f };
	//	//m_inverseViewMatrix[0][0] = u.x;
	//	//m_inverseViewMatrix[0][1] = u.y;
	//	//m_inverseViewMatrix[0][2] = u.z;
	//	//m_inverseViewMatrix[1][0] = v.x;
	//	//m_inverseViewMatrix[1][1] = v.y;
	//	//m_inverseViewMatrix[1][2] = v.z;
	//	//m_inverseViewMatrix[2][0] = w.x;
	//	//m_inverseViewMatrix[2][1] = w.y;
	//	//m_inverseViewMatrix[2][2] = w.z;
	//	//m_inverseViewMatrix[3][0] = position.x;
	//	//m_inverseViewMatrix[3][1] = position.y;
	//	//m_inverseViewMatrix[3][2] = position.z;
	//	m_inverseViewMatrix = glm::inverse(m_viewMatrix);
	//}

	void SceneCamera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		//SetViewDirection(position, target - position, up);
		//const glm::vec3 w{ glm::normalize(direction) };
		//const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
		//const glm::vec3 v{ glm::cross(w, u) };

		m_viewMatrix = glm::lookAt(position, target, up);
		m_inverseViewMatrix = glm::inverse(m_viewMatrix);
	}

	void SceneCamera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation) {
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
		const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
		const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
		m_viewMatrix = glm::mat4{ 1.f };
		m_viewMatrix[0][0] = u.x;
		m_viewMatrix[1][0] = u.y;
		m_viewMatrix[2][0] = u.z;
		m_viewMatrix[0][1] = v.x;
		m_viewMatrix[1][1] = v.y;
		m_viewMatrix[2][1] = v.z;
		m_viewMatrix[0][2] = w.x;
		m_viewMatrix[1][2] = w.y;
		m_viewMatrix[2][2] = w.z;
		m_viewMatrix[3][0] = -glm::dot(u, position);
		m_viewMatrix[3][1] = -glm::dot(v, position);
		m_viewMatrix[3][2] = -glm::dot(w, position);

		m_inverseViewMatrix = glm::mat4{ 1.f };
		m_inverseViewMatrix[0][0] = u.x;
		m_inverseViewMatrix[0][1] = u.y;
		m_inverseViewMatrix[0][2] = u.z;
		m_inverseViewMatrix[1][0] = v.x;
		m_inverseViewMatrix[1][1] = v.y;
		m_inverseViewMatrix[1][2] = v.z;
		m_inverseViewMatrix[2][0] = w.x;
		m_inverseViewMatrix[2][1] = w.y;
		m_inverseViewMatrix[2][2] = w.z;
		m_inverseViewMatrix[3][0] = position.x;
		m_inverseViewMatrix[3][1] = position.y;
		m_inverseViewMatrix[3][2] = position.z;

		//glm::vec3 target = position + glm::vec3(
		//	glm::cos(rotation.y) * glm::cos(rotation.x),
		//	glm::sin(rotation.x),
		//	glm::sin(rotation.y) * glm::cos(rotation.x)
		//);
		//m_viewMatrix = glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
		//m_inverseViewMatrix = glm::inverse(m_viewMatrix);

		//// Calculate rotation matrix from YXZ Euler angles
		//const glm::mat3 rotationMatrix = glm::mat3(
		//	glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * // Y-axis rotation
		//	glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * // X-axis rotation
		//	glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))   // Z-axis rotation
		//);

		//// Extract basis vectors
		//const glm::vec3 u = rotationMatrix[0]; // Right vector
		//const glm::vec3 v = rotationMatrix[1]; // Up vector
		//const glm::vec3 w = rotationMatrix[2]; // Forward vector

		//// Compute view matrix
		//m_viewMatrix = glm::mat4{
		//	glm::vec4(u, 0.0f),
		//	glm::vec4(v, 0.0f),
		//	glm::vec4(w, 0.0f),
		//	glm::vec4(-glm::dot(u, position), -glm::dot(v, position), -glm::dot(w, position), 1.0f)
		//};
		//m_viewMatrix = glm::transpose(m_viewMatrix); // Convert column-major to row-major

		//// Compute inverse view matrix
		//m_inverseViewMatrix = glm::mat4{
		//	glm::vec4(u, 0.0f),
		//	glm::vec4(v, 0.0f),
		//	glm::vec4(w, 0.0f),
		//	glm::vec4(position, 1.0f)
		//};
	}
}  // namespace RVK
