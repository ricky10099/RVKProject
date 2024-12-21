#pragma once

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	static constexpr int NO_PARENT = -1;
	static constexpr int ROOT_JOINT = 0;

	struct ShaderData {
		std::vector<glm::mat4> finalJointsMatrices;
	};

	struct Joint {
		std::string_view name;
		glm::mat4 inverseBindMatrix;

		glm::vec3 deformedNodeTranslate{ 0.0f };
		glm::quat deformedNodeRotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 deformedNodeScale{ 1.0f };

		int parentJoint;
		std::vector<int> children;

		glm::mat4 GetDeformedBindMatrix() {
			return glm::translate(glm::mat4{ 1.0f }, deformedNodeTranslate) *
				glm::mat4(deformedNodeRotation) *
				glm::scale(glm::mat4{ 1.0f }, deformedNodeScale);
		}
	};

	struct Skeleton {
		bool isAnimated = true;
		std::string name;
		std::vector<Joint> joints;
		std::map<int, int> globalNodeToJointIndex;
		ShaderData shaderData;

		void Traverse();
		void Traverse(const Joint& joint, u32 indent = 0);
		void Update();
		void UpdateJoint(s16 joint);
	};
}