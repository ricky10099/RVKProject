#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Framework/Utils.h"

#define VK_CHECK(x, msg) if (x != VK_SUCCESS) { VK_CORE_ERROR(msg); }
#define MAX_INSTANCE 64

namespace RVK {
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    //const int MAX_OBJECTS = 20;

    const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME
    };

    static std::vector<char> ReadFile(const std::string& filename) {
        // Open stream from given file
        // std::ios::binary tells stream to read file as binary
        // std::ios::ate tells stream to start reading from the and of file
        std::string enginePath = ENGINE_DIR + filename;

        std::ifstream file(enginePath, std::ios::binary | std::ios::ate);

        // Check if file stream successfully opened
        if (!file.is_open()) {
            VK_CORE_ERROR("Failed to open a file: " + enginePath);
        }

        // Get current read position and use to resize file buffer
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> fileBuffer(fileSize);

        // Move read position (seek to) the start of the file
        file.seekg(0);

        // Read the file data into the buffer(stream "fileSize" in total)
        file.read(fileBuffer.data(), fileSize);

        // Close stream
        file.close();

        return fileBuffer;
    }
}