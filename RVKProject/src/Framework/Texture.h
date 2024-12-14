#pragma once

#include "Framework/Utils.h"

namespace RVK {
	class Texture {
	public:
		static constexpr bool USE_SRGB = true;
		static constexpr bool USE_UNORM = false;

	public:
		bool Init(const u32 width, const u32 height, bool sRGB, const void* data, int minFilter, int magFilter);
		bool Init(const std::string& fileName, bool sRGB, bool flip = true);
	};
}