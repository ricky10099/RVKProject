#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Materials.h"
#include "Framework/Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
			std::vector<u32> indices{};

			void LoadModel(const std::string& filepath);
		};
		 
		struct AssimpLoader {
			std::vector<Vertex> vertices{};
			std::vector<u32> indices{};
			std::vector<Material> m_materials;
			std::vector<Material::MaterialTextures> m_materialTextures{};
			std::vector<std::shared_ptr<Texture>> m_textures;

			void LoadModel(std::string_view filepath);
			void LoadMaterials(const aiScene* scene);
			void LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial);
			void LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex);
			//void MarkNode(const aiNode* fbxNodePtr, const aiScene* scene);
			void ProcessNode(const aiNode* fbxNodePtr);
			std::shared_ptr<Texture> LoadTexture(std::string const& filepath, bool useSRGB);
		};

		Model(const Model::Builder& builder);
		~Model();

		NO_COPY(Model)

		static std::unique_ptr<Model> CreateModelFromFile(const std::string& filepath);

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

	private:
		std::unique_ptr<RVKBuffer> m_vertexBuffer;
		u32 m_vertexCount;

		bool m_hasIndexBuffer = false;
		std::unique_ptr<RVKBuffer> m_indexBuffer;
		u32 m_indexCount;

	private:
		void CreateVertexBuffers(const std::vector<Vertex>& vertices);
		void CreateIndexBuffers(const std::vector<u32>& indices);
	};
}  // namespace RVK
