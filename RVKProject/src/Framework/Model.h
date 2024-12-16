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
	struct Submesh
	{
		u32 firstIndex;
		u32 firstVertex;
		u32 indexCount;
		u32 vertexCount;
		//u32 instanceCount;
		//Material m_Material;
		//Resources m_Resources;
	};

	class Model {
	public:
		enum class ModelType {
			TinyObj,
			Ufbx,
			Assimp,
		};

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

		struct TinyObjBuilder {
			std::vector<Vertex> vertices{};
			std::vector<u32> indices{};

			void LoadModel(const std::string& filepath);
		};

		struct ufbxBuilder {
			std::vector<Vertex> vertices{};
			std::vector<u32> indices{};

			void LoadModel(const std::string& filepath);
			void LoadVertexData(const ufbx_node* fbxNodePtr, const u32 idx);
			void convert_mesh_part(ufbx_mesh* mesh, ufbx_mesh_part* part);
		};

		struct AssimpBuilder {
			std::vector<Vertex> vertices{};
			std::vector<u32> indices{};
			std::vector<Submesh> submeshes{};
			const aiScene* m_scene{};
			bool m_FbxNoBuiltInTangents;

			void LoadModel(const std::string& filepath);
			void ProcessNode(aiNode* node, const aiScene* scene);
			void ProcessMesh(aiMesh* mesh, const aiScene* scene);
			void CreateObject(const aiNode* fbxNodePtr);
			void LoadVertexData(const aiNode* fbxNodePtr, int vertexColorSet = 0, u32 uvSet = 0);
			void LoadVertexData(const aiNode* fbxNodePtr, u32 const meshIndex, u32 const fbxMeshIndex, int vertexColorSet = 0,
				u32 uvSet = 0);
			void CalculateTangents();
			void CalculateTangentsFromIndexBuffer(const std::vector<u32>& indices);
		};

		Model(const Model::TinyObjBuilder& builder);
		Model(const Model::ufbxBuilder& builder);
		Model(const Model::AssimpBuilder& builder);
		~Model();

		NO_COPY(Model)

		static std::unique_ptr<Model> CreateModelFromFile(const std::string& filepath, ModelType type = ModelType::Ufbx);

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
