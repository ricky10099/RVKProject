#include "Framework/Animation.h"
#include "Framework/Timestep.h"

namespace RVK {
	Animation::Animation(/*std::string_view name*/): /*m_name(name),*/ Animation(false){}

	Animation::Animation(bool isLoop) : m_isLoop(false){}

	void Animation::Start() {
		m_currentKeyFrameTime = m_firstKeyFrameTime;
	}

	void Animation::Stop() {
		m_currentKeyFrameTime = m_lastKeyFrameTime + 1.0f;
	}

	bool Animation::IsRunning() const {
		return (m_isLoop || (m_currentKeyFrameTime <= m_lastKeyFrameTime));
	}

	bool Animation::WillExpire(const Timestep& timestep) const {
		return (!m_isLoop && ((m_currentKeyFrameTime + timestep) > m_lastKeyFrameTime));
	}

	void Animation::Update(const Timestep& timestep, Skeleton& skeleton) {
		if (!IsRunning()) {
			//VK_CORE_WARN("Animation '{0}' expired", m_name);
			return;
		}

		m_currentKeyFrameTime += timestep;

		if (m_isLoop && (m_currentKeyFrameTime > m_lastKeyFrameTime)) {
			m_currentKeyFrameTime = m_firstKeyFrameTime;
		}

		for (auto& channel : m_channels) {
			auto& sampler = m_samplers[channel.samplerIndex];
			int jointIndex = skeleton.globalNodeToJointIndex[channel.node];
			auto& joint = skeleton.joints[jointIndex];

			for (size_t i = 0; i < sampler.timestamps.size() - 1; ++i) {
				if ((m_currentKeyFrameTime >= sampler.timestamps[i])
					&& (m_currentKeyFrameTime <= sampler.timestamps[i + 1])) {
					switch (sampler.interpolation) {
					case InterpolationMethod::LINEAR: {
						float a = (m_currentKeyFrameTime - sampler.timestamps[i])
							/ (sampler.timestamps[i + 1] - sampler.timestamps[i]);
						switch (channel.path) {
						case Path::TRANSLATION: {
							joint.deformedNodeTranslate = glm::mix(sampler.transformToInterpolate[i],
								sampler.transformToInterpolate[i + 1], a);
							break;
						}
						case Path::ROTATION: {
							glm::quat quaternion1;
							quaternion1.x = sampler.transformToInterpolate[i].x;
							quaternion1.y = sampler.transformToInterpolate[i].y;
							quaternion1.z = sampler.transformToInterpolate[i].z;
							quaternion1.w = sampler.transformToInterpolate[i].w;

							glm::quat quaternion2;
							quaternion2.x = sampler.transformToInterpolate[i].x;
							quaternion2.y = sampler.transformToInterpolate[i].y;
							quaternion2.z = sampler.transformToInterpolate[i].z;
							quaternion2.w = sampler.transformToInterpolate[i].w;

							joint.deformedNodeRotation = glm::normalize(glm::slerp(quaternion1, quaternion2, a));
							break;
						}
						case Path::SCALE: {
							joint.deformedNodeScale = glm::mix(sampler.transformToInterpolate[i],
								sampler.transformToInterpolate[i + 1], a);
							break;
						}
						default:
							VK_CORE_CRITICAL("Path not found!");
						}
						break;
					}
					case InterpolationMethod::STEP: {
						switch (channel.path) {
						case Path::TRANSLATION: {
							joint.deformedNodeTranslate = glm::vec3(sampler.transformToInterpolate[i]);
							break;
						}
						case Path::ROTATION: {
							joint.deformedNodeRotation.x = sampler.transformToInterpolate[i].x;
							joint.deformedNodeRotation.y = sampler.transformToInterpolate[i].y;
							joint.deformedNodeRotation.z = sampler.transformToInterpolate[i].z;
							joint.deformedNodeRotation.w = sampler.transformToInterpolate[i].w;
							break;
						}
						case Path::SCALE: {
							joint.deformedNodeScale = glm::vec3(sampler.transformToInterpolate[i]);
							break;
						}
						default:
							VK_CORE_CRITICAL("Path not found!");
						}
						break;
					}
					case InterpolationMethod::CUBICSPLINE: {
						VK_CORE_WARN("Animation::Update(): interpolation method CUBICSPLINE not supported!");
						break;
					}
					default:
						VK_CORE_WARN("Animation::Update(): unknown interpolation method!");
						break;
					}
				}
			}
		}
	}
}// namespace RVK