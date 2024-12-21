#pragma once

#include "Framework/skeleton.h"

namespace RVK {
	class Timestep;
	class Animation {
	public:
		enum class Path {
			TRANSLATION,
			ROTATION,
			SCALE,
		};

		enum class InterpolationMethod {
			LINEAR,
			STEP,
			CUBICSPLINE,
		};

		struct Channel {
			Path path;
			int samplerIndex;
			int node;
		};

		struct Sampler {
			std::vector<float> timestamps;
			std::vector<glm::vec4> transformToInterpolate;
			InterpolationMethod interpolation;
		};

	public:
		Animation(/*std::string_view name*/);
		Animation(bool isLoop);

		void Start();
		void Stop();
		bool IsRunning() const;
		bool WillExpire(const Timestep& timestep) const;
		void SetLoop(bool isLoop) { m_isLoop = isLoop; }
		void Update(const Timestep& timestep, Skeleton& skeleton);

		std::vector<Sampler> m_samplers;
		std::vector<Channel> m_channels;

		void SetFirstKeyFrameTime(float firstKeyFrameTime) { m_firstKeyFrameTime = firstKeyFrameTime; }
		void SetLastKeyFrameTime(float lastKeyFrameTime) { m_lastKeyFrameTime = lastKeyFrameTime; }

		//std::string_view GetName() const { return m_name; }
		float GetCurrentAnimTime() const { return m_currentKeyFrameTime - m_firstKeyFrameTime; }
		float GetAnimDuration() const { return m_lastKeyFrameTime - m_firstKeyFrameTime; }

	private:
		//std::string_view m_name;
		bool m_isLoop;

		float m_firstKeyFrameTime;
		float m_lastKeyFrameTime;
		float m_currentKeyFrameTime = 0.0f;
	};
}// namespace RVK