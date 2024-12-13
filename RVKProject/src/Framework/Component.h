#pragma once

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Framework/Utils.h"
#include "Framework/Model.h"
#include "Framework/Camera.h"


namespace RVK::Components {
	struct Tag {
		std::string tag;

		Tag() = default;
		Tag(const Tag&) = default;
		Tag(const std::string& tag)
			: tag(tag) {}
	};

	struct Transform {
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		Transform() = default;
		Transform(const Transform&) = default;
		Transform(const glm::vec3& pos)
			: position(pos) {}

		glm::mat4 GetTransform() const {
			return glm::translate(glm::mat4(1.0f), position)*
				glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))*
				glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f))*
				glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f))*
				glm::scale(glm::mat4(1.0f), scale);
		}

		glm::mat3 NormalMatrix() const {
			return glm::mat3_cast(glm::quat(rotation)) * (glm::mat3)glm::scale(glm::mat4(1.0f), 1.0f / scale);
		}

		void Translate(const glm::vec3& pos) { position += pos; }
	};

	struct Mesh {
		std::shared_ptr<Model> model;

		Mesh() = default;
		Mesh(const Mesh&) = default;
		Mesh(const std::string& path)
			: model(Model::CreateModelFromFile(path)) {}
	};

	struct Camera {
		SceneCamera camera;
		bool currentCamera = false;
		bool fixedAspectRatio = false;

		Camera() = default;
		Camera(const Camera&) = default;
	};

	struct PointLight {
		glm::vec3 color;
		float lightIntensity;
		float radius;

		PointLight() : PointLight({ 1.0f, 1.0f, 1.0f }) {};
		PointLight(const PointLight&) = default;
		PointLight(glm::vec3 col, float intensity = 0.2f, float radius = 0.1f) {
			color = col;
			lightIntensity = intensity;
			this->radius = radius;
		}
	};

}