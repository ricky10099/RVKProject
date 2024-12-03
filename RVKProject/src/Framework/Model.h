#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKBuffer.h"

namespace RVK {
    class Model {
    public:
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

            bool operator==(const Vertex& other) const {
                return position == other.position && color == other.color && normal == other.normal &&
                    uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void LoadModel(const std::string& filepath);
        };

        Model(const Model::Builder& builder);
        ~Model();

        NO_COPY(Model)

        static std::unique_ptr<Model> CreateModelFromFile(const std::string& filepath);

        void Bind(VkCommandBuffer commandBuffer);
        void Draw(VkCommandBuffer commandBuffer);

    private:
        void CreateVertexBuffers(const std::vector<Vertex>& vertices);
        void CreateIndexBuffers(const std::vector<uint32_t>& indices);

        std::unique_ptr<RVKBuffer> m_vertexBuffer;
        u32 m_vertexCount;

        bool m_hasIndexBuffer = false;
        std::unique_ptr<RVKBuffer> m_indexBuffer;
        u32 m_indexCount;
    };
}
