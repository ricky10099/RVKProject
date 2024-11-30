#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

namespace RVK {
    class Log {
    public:
        static void Init();
        static void Shutdown() { spdlog::shutdown(); }

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() {
            return m_coreLogger;
        }
        static std::shared_ptr<spdlog::logger>& GetAppLogger() {
            return m_appLogger;
        }

    private:
        static std::shared_ptr<spdlog::logger> m_coreLogger;
        static std::shared_ptr<spdlog::logger> m_appLogger;
    };
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector) {
    return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix) {
    return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion) {
    return os << glm::to_string(quaternion);
}

#define VK_CORE_TRACE(...)        RVK::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VK_CORE_INFO(...)         RVK::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VK_CORE_WARN(...)         RVK::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VK_CORE_ERROR(...)        RVK::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VK_CORE_CRITICAL(...)     RVK::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define VK_TRACE(...)           RVK::Log::GetAppLogger()->trace(__VA_ARGS__)
#define VK_INFO(...)            RVK::Log::GetAppLogger()->info(__VA_ARGS__)
#define VK_WARN(...)            RVK::Log::GetAppLogger()->warn(__VA_ARGS__)
#define VK_ERROR(...)           RVK::Log::GetAppLogger()->error(__VA_ARGS__)
#define VK_CRITICAL(...)        RVK::Log::GetAppLogger()->critical(__VA_ARGS__)
