#pragma once

#include <vector>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Framework/Utils.h"
//#include "Framework/Camera.h"

#define VK_CHECK(x, msg) if (x != VK_SUCCESS) { VK_CORE_ERROR(msg); }
#define MAX_LIGHTS 128

// material
#define GLSL_HAS_DIFFUSE_MAP (0x1 << 0x0)
#define GLSL_HAS_NORMAL_MAP (0x1 << 0x1)
#define GLSL_HAS_ROUGHNESS_MAP (0x1 << 0x2)
#define GLSL_HAS_METALLIC_MAP (0x1 << 0x3)
#define GLSL_HAS_ROUGHNESS_METALLIC_MAP (0x1 << 0x4)
#define GLSL_HAS_EMISSIVE_COLOR (0x1 << 0x5)
#define GLSL_HAS_EMISSIVE_MAP (0x1 << 0x6)

// shader properties
#define GLSL_HAS_INSTANCING (0x1 << 0x0)
#define GLSL_HAS_SKELETAL_ANIMATION (0x1 << 0x1)
#define GLSL_HAS_HEIGHTMAP (0x1 << 0x2)

#define MAX_INSTANCE 64

namespace RVK {
#pragma region VulkanCore
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    //const int MAX_OBJECTS = 20;

    const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    //// Indices (locations) of Queue Familes (if they exist at all)
    //struct QueueFamilyIndices {
    //    u32 graphicsFamily = -1;	// Location of Graphisc Queue Family
    //    u32 presentFamily = -1;	    // Location of Presentation Queue Family

    //    // Check if queue families are valid
    //    bool IsValid() {
    //        return graphicsFamily >= 0 && presentFamily >= 0;
    //    }
    //};

    //struct SwapChainDetails {
    //    VkSurfaceCapabilitiesKHR capabilities{};		// Surface properties, e.g. image size/extent
    //    std::vector<VkSurfaceFormatKHR> formats{};	    // Surface image formats, e.g. RGBA and size of each colour
    //    std::vector<VkPresentModeKHR> presentModes{};	// How images should be presented to screen
    //};
#pragma endregion

#pragma region ShaderData
    //struct PointLight {
    //    glm::vec4 position{}; // ignore w
    //    glm::vec4 color{};    // w is intensity
    //};

    //struct DirectionalLight {
    //    glm::vec4 direction{}; // ignore w
    //    glm::vec4 color{};     // w is intensity
    //};

    //// remember alignment requirements!
    //// https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/app09lev1sec2.html
    //struct GlobalUniformBuffer {
    //    glm::mat4 projection{ 1.0f };
    //    glm::mat4 view{ 1.0f };

    //    // point light
    //    glm::vec4 ambientLightColor{ 0.0f, 0.0f, 0.0f, 0.0f };
    //    PointLight pointLights[MAX_LIGHTS];
    //    DirectionalLight directionalLight;
    //    int numberOfActivePointLights;
    //    int numberOfActiveDirectionalLights;
    //};

    //struct Vertex {
    //    glm::vec3 position{};   // layout(location = 0)
    //    glm::vec3 color{};      // layout(location = 1)
    //    glm::vec3 normal{};     // layout(location = 2)
    //    glm::vec2 uv{};         // layout(location = 3)
    //    glm::vec3 tangent;      // layout(location = 4)
    //    glm::ivec4 jointIds;    // layout(location = 5)
    //    glm::vec4 weights;      // layout(location = 6)

    //    //static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
    //    //static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

    //    bool operator==(const Vertex& other) const {
    //        return position == other.position && color == other.color && normal == other.normal &&
    //            uv == other.uv;
    //    }
    //};

    //struct ShadowUniformBuffer {
    //    glm::mat4 projection{ 1.0f };
    //    glm::mat4 view{ 1.0f };
    //};

    //struct PushConstantData {
    //    glm::mat4 modelMatrix{ 1.0f };
    //    glm::mat4 normalMatrix{ 1.0f }; // 4x4 because of alignment
    //};
#pragma endregion

    //class Camera;
    //class GameObject;
    //struct FrameInfo {
    //    u32 frameIndex;
    //    u32 imageIndex;
    //    float frameTime;
    //    VkCommandBuffer commandBuffer;
    //    Camera* camera;
    //    VkDescriptorSet descriptorSet;
    //    GameObject::Map& gameObjects;
    //};

    //struct PipelineConfig {
    //    PipelineConfig() = default;

    //    NO_COPY(PipelineConfig)

    //    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    //    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    //    VkPipelineViewportStateCreateInfo viewportInfo;
    //    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    //    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    //    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    //    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    //    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    //    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    //    std::vector<VkDynamicState> dynamicStateEnables;
    //    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    //    VkPipelineLayout pipelineLayout = nullptr;
    //    VkRenderPass renderPass = nullptr;
    //    u32 subpass = 0;
    //};

#pragma region RenderSystem
 /*   enum class SubPass3D {
        SUBPASS_GEOMETRY = 0,
        SUBPASS_LIGHTING,
        SUBPASS_TRANSPARENCY,
        NUMBER_OF_SUBPASSES,
    };

    enum class RenderTarget3D {
        ATTACHMENT_COLOR = 0,
        ATTACHMENT_DEPTH,
        ATTACHMENT_GBUFFER_POSITION,
        ATTACHMENT_GBUFFER_NORMAL,
        ATTACHMENT_GBUFFER_COLOR,
        ATTACHMENT_GBUFFER_MATERIAL,
        ATTACHMENT_GBUFFER_EMISSION,
        NUMBER_OF_ATTACHMENTS,
    };

    enum class SubPassPostProcessing {
        SUBPASS_BLOOM = 0,
        NUMBER_OF_SUBPASSES,
    };

    enum class RenderTargetPostProcessing {
        ATTACHMENT_COLOR = 0,
        INPUT_ATTACHMENT_3DPASS_COLOR,
        INPUT_ATTACHMENT_GBUFFER_EMISSION,
        NUMBER_OF_ATTACHMENTS,
    };

    enum class SubPassGUI {
        SUBPASS_GUI = 0,
        NUMBER_OF_SUBPASSES,
    };

    enum class RenderTargetGUI {
        ATTACHMENT_COLOR = 0,
        NUMBER_OF_ATTACHMENTS,
    };*/
#pragma endregion

#pragma region Material
    //enum TextureIndex {
    //    DIFFUSE_MAP_INDEX = 0,
    //    NORMAL_MAP_INDEX,
    //    ROUGHNESS_MAP_INDEX,
    //    METALLIC_MAP_INDEX,
    //    ROUGHNESS_METALLIC_MAP_INDEX,
    //    EMISSIVE_MAP_INDEX,
    //    NUM_TEXTURES,
    //};

    //enum class MaterialFeatures { // bitset
    //    HAS_DIFFUSE_MAP = GLSL_HAS_DIFFUSE_MAP,
    //    HAS_NORMAL_MAP = GLSL_HAS_NORMAL_MAP,
    //    HAS_ROUGHNESS_MAP = GLSL_HAS_ROUGHNESS_MAP,
    //    HAS_METALLIC_MAP = GLSL_HAS_METALLIC_MAP,
    //    HAS_ROUGHNESS_METALLIC_MAP = GLSL_HAS_ROUGHNESS_METALLIC_MAP,
    //    HAS_EMISSIVE_COLOR = GLSL_HAS_EMISSIVE_COLOR,
    //    HAS_EMISSIVE_MAP = GLSL_HAS_EMISSIVE_MAP,
    //};

    //struct PBRMaterial {
    //    // align data to blocks of 16 bytes
    //    // byte 0 to 15
    //    u32 features{ 0 };
    //    float roughness{ 0.0f };
    //    float metallic{ 0.0f };
    //    float spare0{ 0.0f }; // padding

    //    // byte 16 to 31
    //    glm::vec4 diffuseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

    //    // byte 32 to 47
    //    glm::vec3 emissiveColor{ 0.0f, 0.0f, 0.0f };
    //    float emissiveStrength{ 1.0f };

    //    // byte 48 to 63
    //    float normalMapIntensity{ 1.0f };
    //    float spare1{ 0.0f }; // padding
    //    float spare2{ 0.0f }; // padding
    //    float spare3{ 0.0f }; // padding

    //    // byte 64 to 128
    //    glm::vec4 spare4[4];
    //};

    //enum class MaterialType {
    //    PBR = 0,
    //    Cubemap,
    //    Diffuse,
    //    NUM_TYPES,
    //};
#pragma endregion

#pragma region Resource
    //enum BufferIndex {
    //    INSTANCE_BUFFER_INDEX = 0,
    //    SKELETAL_ANIMATION_BUFFER_INDEX,
    //    HEIGHTMAP,
    //    MULTI_PURPOSE_BUFFER,
    //    NUM_BUFFERS,
    //};

    //enum ResourceFeatures {  //bitset        
    //    HAS_INSTANCING = GLSL_HAS_INSTANCING,
    //    HAS_SKELETAL_ANIMATION = GLSL_HAS_SKELETAL_ANIMATION,
    //    HAS_HEIGHTMAP = GLSL_HAS_HEIGHTMAP,
    //};
#pragma endregion

    static std::vector<char> ReadFile(const std::string& filename) {
        // Open stream from given file
        // std::ios::binary tells stream to read file as binary
        // std::ios::ate tells stream to start reading from the and of file
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        // Check if file stream successfully opened
        if (!file.is_open()) {
            VK_CORE_ERROR("Failed to open a file: " + filename);
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