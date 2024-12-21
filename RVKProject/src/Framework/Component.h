#pragma once

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <PxPhysicsAPI.h>

#include "Framework/Utils.h"
#include "Framework/MeshModel.h"
#include "Framework/Camera.h"
#include "Framework/Animation.h"

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
		Transform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& sca)
			: position(pos), rotation(rot), scale(sca) {}

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

	struct Model {
		std::shared_ptr<MeshModel> model;
		Transform offset{ glm::vec3(0.0f) };

		std::map<std::string_view, std::shared_ptr<Animation>> animations;

		Model() = default;
		Model(const Model&) = default;
		Model(const std::string& path)
			: model(MeshModel::CreateMeshModelFromFile(path)) {}
		void SetOffsetPosition(const glm::vec3& pos) {
			offset.position = pos;
		}
		void SetOffsetRotation(const glm::vec3& rot) {
			offset.rotation = rot;
		}
		void SetOffetScale(const glm::vec3& scale) {
			offset.scale = scale;
		}

		void AddAnimation(std::string_view name, const std::string& animationPath);
	};

	struct Camera {
		SceneCamera camera;
		bool currentCamera = false;
		bool fixedAspectRatio = false;

		Camera() = default;
		Camera(const Camera&) = default;
		Camera(bool curr) : currentCamera(curr) {}
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
	
	//struct RigidBody {
	//	physx::PxRigidBody* rigidBody;
	//	Transform offset{glm::vec3(0.0f)};

	//	RigidBody() = default;
	//	RigidBody(const RigidBody&) = default;
	//	RigidBody(const glm::vec3& pos, bool isStatic = true): RigidBody(pos, glm::vec3(0.0f), glm::vec3(1.0f), isStatic) {}
	//	RigidBody(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale, bool isStatic) {
	//		rigidBody = 
	//	}
	//};

}