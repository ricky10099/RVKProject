#include "Framework/Component.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace RVK::Components {
	void Model::AddAnimation(std::string_view name, const std::string& animationPath) {
		// Import model "scene"
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath,
			aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
			aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
		if (!scene) {
			VK_CORE_ERROR("Failed to load model! ({0})", animationPath);
		}

        u32 numberOfSkeletons = 0;
        u32 meshIndex = 0;
        // iterate over all meshes and check if they have a skeleton
        for (u32 index = 0; index < scene->mNumMeshes; ++index)
        {
            aiMesh* mesh = scene->mMeshes[index];
            if (mesh->mNumBones)
            {
                ++numberOfSkeletons;
                meshIndex = index;
            }
        }
        if (!numberOfSkeletons)
        {
            return;
        }

        if (numberOfSkeletons > 1)
        {
            VK_CORE_WARN("A model should only have a single skin/armature/skeleton. Using skin {0}.",
                numberOfSkeletons - 1);
        }

        std::unordered_map<std::string, int> nameToBoneIndex;

        // load skeleton
        {

            aiMesh* mesh = scene->mMeshes[meshIndex];
            size_t numberOfJoints = mesh->mNumBones;
            //auto& joints =
            //    m_Skeleton->m_Joints; // just a reference to the joints std::vector of that skeleton (to make code easier)

            //joints.resize(numberOfJoints);
            //m_Skeleton->m_ShaderData.m_FinalJointsMatrices.resize(numberOfJoints);

            // set up map to find the names of bones when traversing the node hierarchy
            // by iterating the mBones array of the mesh
            for (u32 boneIndex = 0; boneIndex < numberOfJoints; ++boneIndex)
            {
                aiBone* bone = mesh->mBones[boneIndex];
                std::string boneName = bone->mName.C_Str();
                nameToBoneIndex[boneName] = boneIndex;

                // compatibility code with glTF loader; needed in skeletalAnimation.cpp
                // m_Channels.m_Node must be set up accordingly
                //m_Skeleton->m_GlobalNodeToJointIndex[boneIndex] = boneIndex;
            }

        }

        size_t numberOfAnimations = scene->mNumAnimations;
        for (size_t animationIndex = 0; animationIndex < numberOfAnimations; ++animationIndex)
        {
            aiAnimation& fbxAnimation = *scene->mAnimations[animationIndex];

            std::string animationName(fbxAnimation.mName.C_Str());
            // the asset importer includes animations twice,
            // as "armature|name" and "name"
            if (animationName.find("|") != std::string::npos)
            {
                continue;
            }
            std::shared_ptr<Animation> animation = std::make_shared<Animation>();

            // animation speed
            double ticksPerSecond = 0.0;
            if (fbxAnimation.mTicksPerSecond > std::numeric_limits<float>::epsilon())
            {
                ticksPerSecond = fbxAnimation.mTicksPerSecond;
            }
            else
            {
                VK_CORE_ERROR("No speed information found in fbx file!");
                ticksPerSecond = 30.0;
            }

            {
                u32 channelAndSamplerIndex = 0;
                u32 numberOfFbxChannels = fbxAnimation.mNumChannels;
                for (u32 fbxChannelIndex = 0; fbxChannelIndex < numberOfFbxChannels; ++fbxChannelIndex)
                {
                    aiNodeAnim& fbxChannel = *fbxAnimation.mChannels[fbxChannelIndex];
                    std::string fbxChannelName(fbxChannel.mNodeName.C_Str());

                    // use fbx channels that actually belong to bones
                    bool isBone = nameToBoneIndex.contains(fbxChannelName);
                    if (isBone)
                    {
                        // helper lambdas to convert asset importer formats to glm
                        auto vec3AssetImporterToGlm = [](aiVector3D const& vec3AssetImporter)
                            { return glm::vec3(vec3AssetImporter.x, vec3AssetImporter.y, vec3AssetImporter.z); };

                        auto quaternionAssetImporterToGlmVec4 = [](aiQuaternion const& quaternionAssetImporter)
                            {
                                glm::vec4 vec4GLM;
                                vec4GLM.x = quaternionAssetImporter.x;
                                vec4GLM.y = quaternionAssetImporter.y;
                                vec4GLM.z = quaternionAssetImporter.z;
                                vec4GLM.w = quaternionAssetImporter.w;

                                return vec4GLM;
                            };

                        // Each node of the skeleton has channels that point to samplers
                        { // set up channels
                            {
                                Animation::Channel channel{};
                                channel.path = Animation::Path::TRANSLATION;
                                channel.samplerIndex = channelAndSamplerIndex + 0;
                                channel.node = nameToBoneIndex[fbxChannelName];

                                animation->m_channels.push_back(channel);
                            }
                            {
                                Animation::Channel channel{};
                                channel.path = Animation::Path::ROTATION;
                                channel.samplerIndex = channelAndSamplerIndex + 1;
                                channel.node = nameToBoneIndex[fbxChannelName];

                                animation->m_channels.push_back(channel);
                            }
                            {
                                Animation::Channel channel{};
                                channel.path = Animation::Path::SCALE;
                                channel.samplerIndex = channelAndSamplerIndex + 2;
                                channel.node = nameToBoneIndex[fbxChannelName];

                                animation->m_channels.push_back(channel);
                            }
                        }

                        { // set up samplers
                            {
                                u32 numberOfKeys = fbxChannel.mNumPositionKeys;

                                Animation::Sampler sampler;
                                sampler.timestamps.resize(numberOfKeys);
                                sampler.transformToInterpolate.resize(numberOfKeys);
                                sampler.interpolation = Animation::InterpolationMethod::LINEAR;
                                for (u32 key = 0; key < numberOfKeys; ++key)
                                {
                                    aiVector3D& value = fbxChannel.mPositionKeys[key].mValue;
                                    sampler.transformToInterpolate[key] =
                                        glm::vec4(vec3AssetImporterToGlm(value), 0.0f);
                                    sampler.timestamps[key] = fbxChannel.mPositionKeys[key].mTime / ticksPerSecond;
                                }

                                animation->m_samplers.push_back(sampler);
                            }
                            {
                                u32 numberOfKeys = fbxChannel.mNumRotationKeys;

                                Animation::Sampler sampler;
                                sampler.timestamps.resize(numberOfKeys);
                                sampler.transformToInterpolate.resize(numberOfKeys);
                                sampler.interpolation = Animation::InterpolationMethod::LINEAR;
                                for (u32 key = 0; key < numberOfKeys; ++key)
                                {
                                    aiQuaternion& value = fbxChannel.mRotationKeys[key].mValue;
                                    sampler.transformToInterpolate[key] = quaternionAssetImporterToGlmVec4(value);
                                    sampler.timestamps[key] = fbxChannel.mPositionKeys[key].mTime / ticksPerSecond;
                                }

                                animation->m_samplers.push_back(sampler);
                            }
                            {
                                u32 numberOfKeys = fbxChannel.mNumScalingKeys;

                                Animation::Sampler sampler;
                                sampler.timestamps.resize(numberOfKeys);
                                sampler.transformToInterpolate.resize(numberOfKeys);
                                sampler.interpolation = Animation::InterpolationMethod::LINEAR;
                                for (u32 key = 0; key < numberOfKeys; ++key)
                                {
                                    aiVector3D& value = fbxChannel.mScalingKeys[key].mValue;
                                    sampler.transformToInterpolate[key] =
                                        glm::vec4(vec3AssetImporterToGlm(value), 0.0f);
                                    sampler.timestamps[key] = fbxChannel.mPositionKeys[key].mTime / ticksPerSecond;
                                }

                                animation->m_samplers.push_back(sampler);
                            }
                        }
                        channelAndSamplerIndex += 3;
                    }
                }
            }

            if (animation->m_samplers.size()) // at least one sampler found
            {
                auto& sampler = animation->m_samplers[0];
                if (sampler.timestamps.size() >= 2) // samplers have at least 2 keyframes to interpolate in between
                {
                    animation->SetFirstKeyFrameTime(sampler.timestamps[0]);
                    animation->SetLastKeyFrameTime(sampler.timestamps.back());
                }
            }

            animations[name] = animation;
        }
	}
}