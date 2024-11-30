#pragma once
#include "rvkpch.h"

#define EXPAND_MACRO(x) x
#define STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define BIND_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#define NO_COPY(type) type(const type&) = delete; type& operator=(const type&) = delete;
#define NO_MOVE(type) type(type&&) = delete; type& operator=(type&&) = delete;

using s8 = char;
using s16 = short;
using s32 = int;
using s64 = long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

#include "Framework/Log.h"
#include "Framework/Assert.h"