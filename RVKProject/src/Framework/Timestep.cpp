#include "Framework/Timestep.h"

namespace RVK {
	Timestep::Timestep(std::chrono::duration<float, std::chrono::seconds::period> time)
		: timestep(time){}

    Timestep& Timestep::operator=(const std::chrono::duration<float, std::chrono::seconds::period>& timestep){
        this->timestep = timestep;
        return *this;
    }

    Timestep& Timestep::operator-=(const Timestep& other){
        timestep = timestep - other.timestep;
        return *this;
    }

    Timestep Timestep::operator-(const Timestep& other) const {
        return timestep - other.timestep; 
    }

    bool Timestep::operator<=(const std::chrono::duration<float, std::chrono::seconds::period>& other) const
    {
        return (timestep - other) <= 0ms;
    }

    std::chrono::duration<float, std::chrono::seconds::period> Timestep::GetSeconds() const { return timestep; }

    std::chrono::duration<float, std::chrono::milliseconds::period> Timestep::GetMilliseconds() const {
        return (std::chrono::duration<float, std::chrono::milliseconds::period>)timestep;
    }

    void Timestep::Print() const {
        auto inMilliSeconds = GetMilliseconds();
        VK_CORE_INFO("timestep in milli seconds: {0} ms", inMilliSeconds.count());
        auto inSeconds = GetSeconds();
        VK_CORE_INFO("timestep in seconds: {0} s", inSeconds.count());
    }
}