#include "Framework/Skeleton.h"

namespace RVK {
	void Skeleton::Traverse() {
		u32 indent = 0;
		std::string indentStr(indent, ' ');
		auto& joint = joints[0]; // root joint
		Traverse(joint, indent + 1);
	}

	void Skeleton::Traverse(const Joint& joint, u32 indent) {
		std::string indentStr(indent, ' ');
		size_t numberOfChildren = joint.children.size();
		VK_CORE_INFO("{0}name: {1}, m_Parent: {2}, children.size(): {3}", indentStr, joint.name,
			joint.parentJoint, numberOfChildren);
		for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex){
			int jointIndex = joint.children[childIndex];
			VK_CORE_INFO("{0}child {1}: index: {2}", indentStr, childIndex, jointIndex);
		}

		for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
			int jointIndex = joint.children[childIndex];
			Traverse(joints[jointIndex], indent + 1);
		}
	}

    void Skeleton::Update() {
        // update the final global transform of all joints
        s16 numberOfJoints = static_cast<s16>(joints.size());

        // used for debugging to check if the model renders w/o deformation
        if (!isAnimated) {
            for (s16 jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex)
            {
                shaderData.finalJointsMatrices[jointIndex] = glm::mat4(1.0f);
            }
        }
        else
        {
            // STEP 1: apply animation results
            for (s16 jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex)
            {
                shaderData.finalJointsMatrices[jointIndex] = joints[jointIndex].GetDeformedBindMatrix();
            }

            // STEP 2: recursively update final joint matrices
            UpdateJoint(ROOT_JOINT);

            // STEP 3: bring back into model space
            for (s16 jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex)
            {
                shaderData.finalJointsMatrices[jointIndex] =
                    shaderData.finalJointsMatrices[jointIndex] * joints[jointIndex].inverseBindMatrix;
            }
        }
    }

    // Update the final joint matrices of all joints
    // traverses entire skeleton from top (a.k.a root a.k.a hip bone)
    // This way, it is guaranteed that the global parent transform is already updated
    void Skeleton::UpdateJoint(s16 jointIndex)
    {
        auto& currentJoint = joints[jointIndex]; // just a reference for easier code

        s16 parentJoint = currentJoint.parentJoint;
        if (parentJoint != NO_PARENT)
        {
            shaderData.finalJointsMatrices[jointIndex] =
                shaderData.finalJointsMatrices[parentJoint] * shaderData.finalJointsMatrices[jointIndex];
        }

        // update children
        size_t numberOfChildren = currentJoint.children.size();
        for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex)
        {
            int childJoint = currentJoint.children[childIndex];
            UpdateJoint(childJoint);
        }
    }
}