#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Materials.h"
#include "Framework/Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "ufbx/ufbx.h"

namespace RVK {
	struct Vertex {
		glm::vec3 position{};
		glm::vec4 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		bool operator==(const Vertex& other) const {
			return position == other.position && color == other.color && normal == other.normal &&
				uv == other.uv;
		}
	};

	struct Mesh
	{
		u32 firstIndex;
		u32 firstVertex;
		u32 indexCount;
		u32 vertexCount;
		//u32 instanceCount;
		Material material;
		//VkDescriptorSet samplerDescriptorSet;


		//Resources m_Resources;
		//std::unique_ptr<RVKBuffer> m_vertexBuffer;
		//u32 m_vertexCount;

		//bool m_hasIndexBuffer = false;
		//std::unique_ptr<RVKBuffer> m_indexBuffer;
		//u32 m_indexCount;
		//void CreateVertexBuffers(const std::vector<Vertex>& vertices);
		//void CreateIndexBuffers(const std::vector<u32>& indices);
	};

	class MeshModel {
	public:
		struct AssimpBuilder {
			std::vector<Vertex> vertices{};
			std::vector<u32> indices{};
			std::vector<Mesh> meshes{};
			std::vector<Material> materials{};
			//std::vector<std::shared_ptr<Texture>> textures;
			//std::vector<VkDescriptorSet> samplerDescriptorSets;

			void LoadMeshModel(const std::string& filepath);
			void LoadNode(aiNode* node, const aiScene* scene);
			void LoadMesh(aiMesh* mesh, const aiScene* scene, u32 meshIndex);
			void LoadMaterials(const aiScene* scene);
			void LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial);
			void LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex);
			std::shared_ptr<Texture> LoadTexture(std::string const& filepath, bool useSRGB);
			void AssignMaterial(Mesh& submesh, int const materialIndex);
		};

		MeshModel(const MeshModel::AssimpBuilder& builder);
		~MeshModel();

		NO_COPY(MeshModel)

		static std::unique_ptr<MeshModel> CreateMeshModelFromFile(const std::string& filepath);

		void Bind(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout);
		void Draw(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout);
		void DrawMesh(VkCommandBuffer commandBuffer, Mesh mesh);

	private:
		std::vector<Mesh> m_meshesMap{};

		std::unique_ptr<RVKBuffer> m_vertexBuffer;
		u32 m_vertexCount;

		bool m_hasIndexBuffer = false;
		std::unique_ptr<RVKBuffer> m_indexBuffer;
		u32 m_indexCount;

	private:
		void CopyMeshes(std::vector<Mesh> const& meshes);

		void CreateVertexBuffers(const std::vector<Vertex>& vertices);
		void CreateIndexBuffers(const std::vector<u32>& indices);

		void BindDescriptors(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, Mesh& mesh);
		void PushConstantsPbr(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, const Mesh& mesh);
	};
}  // namespace RVK
