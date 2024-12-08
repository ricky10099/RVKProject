#pragma once
#include "rvkpch.h"

#define EXPAND_MACRO(x) x
#define STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define BIND_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#define NO_COPY(type) type(const type&) = delete; type& operator=(const type&) = delete;
#define NO_MOVE(type) type(type&&) = delete; type& operator=(type&&) = delete;
#define CAN_MOVE(type) type(type&&) = default; type& operator=(type&&) = default;

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif


using s8 = char;
using s16 = short;
using s32 = int;
using s64 = long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
	seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
	(HashCombine(seed, rest), ...);
};

#include "Framework/Log.h"
#include "Framework/Assert.h"