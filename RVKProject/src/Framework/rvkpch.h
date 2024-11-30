#pragma once

#ifdef _WIN32
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <Windows.h>
	#undef near
	#undef far
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <chrono>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
