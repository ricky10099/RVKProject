#pragma once
#include <filesystem>

#ifdef VK_DEBUG
#define VK_ENABLE_ASSERTS
#define VK_DEBUGBREAK() __debugbreak()
#endif

#ifdef VK_ENABLE_ASSERTS
#define VK_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { VK##type##ERROR(msg, __VA_ARGS__); VK_DEBUGBREAK(); } }
#define VK_INTERNAL_ASSERT_WITH_MSG(type, check, ...) VK_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define VK_INTERNAL_ASSERT_NO_MSG(type, check) VK_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define VK_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define VK_INTERNAL_ASSERT_GET_MACRO(...) EXPAND_MACRO( VK_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, VK_INTERNAL_ASSERT_WITH_MSG, VK_INTERNAL_ASSERT_NO_MSG) )

#define VK_ASSERT(...) EXPAND_MACRO( VK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define VK_CORE_ASSERT(...) EXPAND_MACRO( VK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#define VK_ASSERT(...)
#define VK_CORE_ASSERT(...)
#endif
