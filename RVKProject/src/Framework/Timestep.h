#pragma once

#include "Framework/Utils.h"

namespace RVK {
	class Timestep {
	public:
		Timestep(std::chrono::duration<float, std::chrono::seconds::period> time);

		Timestep& operator=(const std::chrono::duration<float, std::chrono::seconds::period>& timestep);
		Timestep& operator-=(const Timestep& other);
		Timestep operator-(const Timestep& other) const;
		bool operator<=(const std::chrono::duration<float, std::chrono::seconds::period>& other) const;
		operator float() const { return timestep.count(); }

		void Print() const;
		float Count() { return timestep.count(); }
		std::chrono::duration<float, std::chrono::seconds::period> GetSeconds() const;
		std::chrono::duration<float, std::chrono::milliseconds::period> GetMilliseconds() const;

	private:
		std::chrono::duration<float, std::chrono::seconds::period> timestep;
	};
}