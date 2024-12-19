#include "Framework/MeshModel.h"
#include "Framework/Vulkan/RVKDevice.h"
#include "Framework/Vulkan/MaterialDescriptor.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
	template <>
	struct hash<RVK::Vertex> {
		size_t operator()(RVK::Vertex const& vertex) const {
			size_t seed = 0;
			HashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}  // namespace std

namespace RVK {
	MeshModel::MeshModel(const MeshModel::AssimpBuilder& builder) {
		CopyMeshes(builder.meshes);
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	MeshModel::~MeshModel() {}

	std::unique_ptr<MeshModel> MeshModel::CreateMeshModelFromFile(const std::string& filepath) {
		AssimpBuilder builder{};
		builder.LoadMeshModel(ENGINE_DIR + filepath);
		return std::make_unique<MeshModel>(builder);
	}

	void MeshModel::CopyMeshes(std::vector<Mesh> const& meshes) {
		for (auto& mesh : meshes) {
			m_meshesMap.push_back(mesh);
		}
	}

	void MeshModel::CreateVertexBuffers(const std::vector<Vertex>& vertices) {
		m_vertexCount = static_cast<u32>(vertices.size());
		VK_ASSERT(m_vertexCount >= 3, "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;
		u32 vertexSize = sizeof(vertices[0]);

		RVKBuffer stagingBuffer{
			vertexSize,
			m_vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)vertices.data());

		m_vertexBuffer = std::make_unique<RVKBuffer>(
			vertexSize,
			m_vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		RVKDevice::s_rvkDevice->CopyBuffer(stagingBuffer.GetBuffer(), m_vertexBuffer->GetBuffer(), bufferSize);
	}

	void MeshModel::CreateIndexBuffers(const std::vector<u32>& indices) {
		m_indexCount = static_cast<u32>(indices.size());
		m_hasIndexBuffer = m_indexCount > 0;

		if (!m_hasIndexBuffer) {
			return;
		}

		VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;
		u32 indexSize = sizeof(indices[0]);

		RVKBuffer stagingBuffer{
			indexSize,
			m_indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)indices.data());

		m_indexBuffer = std::make_unique<RVKBuffer>(
			indexSize,
			m_indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		RVKDevice::s_rvkDevice->CopyBuffer(stagingBuffer.GetBuffer(), m_indexBuffer->GetBuffer(), bufferSize);
	}

	void MeshModel::BindDescriptors(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, Mesh& mesh) {

		mesh.material.m_materialBuffer->WriteToBuffer(&mesh.material.m_PBRMaterial);
		mesh.material.m_materialBuffer->Flush();

		const VkDescriptorSet& materialDescriptorSet = mesh.material.m_materialDescriptor->GetDescriptorSet();

		std::vector<VkDescriptorSet> descriptorSets = { frameInfo.globalDescriptorSet, materialDescriptorSet };
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,    // VkCommandBuffer        commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,				// VkPipelineBindPoint    pipelineBindPoint,
			pipelineLayout,									// VkPipelineLayout       layout,
			0,												// uint32_t               firstSet,
			static_cast<u32>(descriptorSets.size()),		// uint32_t               descriptorSetCount,
			descriptorSets.data(),							// const VkDescriptorSet* pDescriptorSets,
			0,												// uint32_t               dynamicOffsetCount,
			nullptr											// const uint32_t*        pDynamicOffsets);
		);
	}

	void MeshModel::PushConstantsPbr(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, const Mesh& mesh) {
		vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(Material::PBRMaterial), &mesh.material.m_PBRMaterial);
	}

	void MeshModel::Draw(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout) {
		for (auto& mesh : m_meshesMap) {
			BindDescriptors(frameInfo, pipelineLayout, mesh);
			DrawMesh(frameInfo.commandBuffer, mesh);
		}
	}

	void MeshModel::DrawMesh(VkCommandBuffer commandBuffer, Mesh mesh) {
		if (m_hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, mesh.firstVertex, 0);
		}
		else {
			vkCmdDraw(commandBuffer, mesh.vertexCount, 1, mesh.firstVertex, 0);
		}
	}
	void MeshModel::Bind(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout) {
		VkBuffer buffers[] = { m_vertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers, offsets);

		if (m_hasIndexBuffer) {
			vkCmdBindIndexBuffer(frameInfo.commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

		return attributeDescriptions;
	}

	void MeshModel::AssimpBuilder::LoadMeshModel(const std::string& filepath) {
		// Import model "scene"
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filepath,
			aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
			aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
		if (!scene) {
			throw std::runtime_error("Failed to load model! (" + filepath + ")");
		}

		LoadMaterials(scene);
		meshes.clear();

		// Load in all our meshes
		LoadNode(scene->mRootNode, scene);
	}

	void MeshModel::AssimpBuilder::LoadMaterials(const aiScene* scene) {
		u32 numMaterials = scene->mNumMaterials;
		materials.resize(numMaterials);
		//samplerDescriptorSets.resize(numMaterials);
		for (u32 materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
			const aiMaterial* fbxMaterial = scene->mMaterials[materialIndex];
			// PrintMaps(fbxMaterial);

			Material& material = materials[materialIndex];

			LoadProperties(fbxMaterial, material.m_PBRMaterial);

			LoadMap(fbxMaterial, aiTextureType_DIFFUSE, materialIndex);
		}
	}

	void MeshModel::AssimpBuilder::LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial)
	{
		{ // diffuse
			aiColor3D diffuseColor;
			if (fbxMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS)
			{
				pbrMaterial.diffuseColor.r = diffuseColor.r;
				pbrMaterial.diffuseColor.g = diffuseColor.g;
				pbrMaterial.diffuseColor.b = diffuseColor.b;
			}
		}
		{ // roughness
			float roughnessFactor;
			if (fbxMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == aiReturn_SUCCESS)
			{
				pbrMaterial.roughness = roughnessFactor;
			}
			else
			{
				pbrMaterial.roughness = 0.1f;
			}
		}

		{ // metallic
			float metallicFactor;
			if (fbxMaterial->Get(AI_MATKEY_REFLECTIVITY, metallicFactor) == aiReturn_SUCCESS)
			{
				pbrMaterial.metallic = metallicFactor;
			}
			else if (fbxMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == aiReturn_SUCCESS)
			{
				pbrMaterial.metallic = metallicFactor;
			}
			else
			{
				pbrMaterial.metallic = 0.886f;
			}
		}

		{ // emissive color
			aiColor3D emission;
			auto result = fbxMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emission);
			if (result == aiReturn_SUCCESS)
			{
				pbrMaterial.emissiveColor = glm::vec3(emission.r, emission.g, emission.b);
			}
		}

		{ // emissive strength
			float emissiveStrength;
			auto result = fbxMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength);
			if (result == aiReturn_SUCCESS)
			{
				pbrMaterial.emissiveStrength = emissiveStrength;
			}
		}

		pbrMaterial.normalMapIntensity = 1.0f;
	}

	void MeshModel::AssimpBuilder::LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex) {
		Material& material = materials[materialIndex];
		Material::PBRMaterial& pbrMaterial = material.m_PBRMaterial;
		Material::MaterialTextures& tmpTextures = material.m_materialTextures;
		
		u32 textureCount = fbxMaterial->GetTextureCount(textureType);
		if (!textureCount) {
			auto texture = LoadTexture("../models/checker.png", Texture::USE_SRGB);
			tmpTextures[Material::DIFFUSE_MAP_INDEX] = texture;
			pbrMaterial.diffuseColor.a = 0.0f;
			return;
		}

		aiString aiFilepath;
		auto getTexture = fbxMaterial->GetTexture(textureType, 0 /* first map*/, &aiFilepath);
		std::string fbxFilepath(aiFilepath.C_Str());
		std::string filepath("../models/" + fbxFilepath);
		if (getTexture == aiReturn_SUCCESS) {
			switch (textureType) {
				// LoadTexture is inside switch statement for sRGB and UNORM
			case aiTextureType_DIFFUSE: {
				auto texture = LoadTexture(filepath, Texture::USE_SRGB);
				if (texture) {
					//textures.push_back(texture);
					tmpTextures[Material::DIFFUSE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_DIFFUSE_MAP;
				}
				break;
			}
			default: {
				VK_CORE_ASSERT(false, "texture type not recognized");
			}
			}
		}
	}

	std::shared_ptr<Texture> MeshModel::AssimpBuilder::LoadTexture(std::string const& filepath, bool useSRGB) {
		std::shared_ptr<Texture> tmpTexture;
		bool loadSucess = false;

		tmpTexture = std::make_shared<Texture>();
		loadSucess = tmpTexture->Init(filepath, useSRGB);

		if (!loadSucess) {
			VK_CORE_CRITICAL("bool FbxBuilder::LoadTexture(): file '{0}' not found", filepath);
			return nullptr;
		}

		//textures.push_back(tmpTexture);
		return tmpTexture;
	}

	void MeshModel::AssimpBuilder::LoadNode(aiNode* node, const aiScene* scene) {
		u32 numMeshes = node->mNumMeshes;

		size_t numMeshesBefore = meshes.size();
		meshes.resize(numMeshes + meshes.size());

		// Go through each mesh at this node and create it, then add it to our meshList
		for (size_t i = 0; i < numMeshes; ++i) {
			LoadMesh(scene->mMeshes[node->mMeshes[i]], scene, numMeshesBefore + i);
		}

		// Go through each node attached to this node and load it, then append their meshes to this mode's mesh list
		for (size_t i = 0; i < node->mNumChildren; ++i) {
			LoadNode(node->mChildren[i], scene);
		}
	}

	void MeshModel::AssimpBuilder::LoadMesh(aiMesh* aimesh, const aiScene* scene, u32 meshIndex) {
		// only triangle mesh supported
		if (!(aimesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) {
			VK_CORE_CRITICAL("FbxBuilder::LoadVertexData: only triangle meshes are supported");
			return;
		}

		const u32 numVertices = aimesh->mNumVertices;
		const u32 numFaces = aimesh->mNumFaces;
		const u32 numIndices = numFaces * 3; // 3 indices per triangle a.k.a face

		size_t numVerticesBefore = vertices.size();
		size_t numIndicesBefore = indices.size();

		// Resize vertex list to hold all vertices for mesh
		vertices.resize(numVerticesBefore + numVertices);
		indices.resize(numIndicesBefore + numIndices);

		Mesh& mesh = meshes[meshIndex];
		mesh.firstVertex = numVerticesBefore;
		mesh.firstIndex = numIndicesBefore;
		mesh.vertexCount = numVertices;
		mesh.indexCount = numIndices;
		//mesh.instanceCount = m_InstanceCount;

		u32 vertexIndex = numVerticesBefore;

		// Go through each vertex and copy it across to our vertices
		for (size_t i = 0; i < aimesh->mNumVertices; ++i) {
			Vertex& vertex = vertices[vertexIndex];

			// Set position
			vertex.position = { aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z };

			// Set normal
			vertex.normal = { aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z };

			//Set uv coords (if they exist)
			if (aimesh->mTextureCoords[0]) {
				vertex.uv = { aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y };
			}
			else {
				vertex.uv = { 0.0f, 0.0f };
			}

			// Set colour (just use white for now)
			//vertex.color = { 1.0f, 1.0f, 1.0f ,1.0f};

			// vertex colors
			{
				glm::vec4 vertexColor;
				u32 materialIndex = aimesh->mMaterialIndex;
				if (aimesh->HasVertexColors(0)) {
					aiColor4D& colorFbx = aimesh->mColors[0][i];
					glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.r, colorFbx.g, colorFbx.b), glm::vec3(2.2f));
					vertexColor = glm::vec4(linearColor.r, linearColor.g, linearColor.b, colorFbx.a);
					vertex.color = vertexColor * materials[materialIndex].m_PBRMaterial.diffuseColor;
				}
				else {
					vertex.color = materials[materialIndex].m_PBRMaterial.diffuseColor;
				}
			}

			++vertexIndex;
		}

		u32 index = numIndicesBefore;
		// Iterate over indices through faces and copy across
		for (size_t i = 0; i < aimesh->mNumFaces; ++i) {
			// Get a face
			aiFace face = aimesh->mFaces[i];

			// Go thorugh face's indices and add to list
			for (size_t j = 0; j < face.mNumIndices; ++j) {
				indices[index + j] = face.mIndices[j];
			}
			index += 3;
		}

		VK_CORE_INFO("mesh loaded (Assimp): {0} vertices, {1} indices", numVertices, numIndices);

		int materialIndex = aimesh->mMaterialIndex;
		if (!(static_cast<size_t>(materialIndex) < materials.size())) {
			VK_CORE_CRITICAL("AssignMaterial: materialIndex must be less than m_Materials.size()");
		}
		if (materialIndex != -1) {
			mesh.material = materials[materialIndex];
			mesh.material.m_materialDescriptor = std::make_shared<MaterialDescriptor>(mesh.material, mesh.material.m_materialTextures);
		}

		VK_CORE_INFO("material assigned (Assimp): material index {0}", materialIndex);
	}

	void MeshModel::AssimpBuilder::AssignMaterial(Mesh& submesh, int const materialIndex) {
		// material
		{
			if (!(static_cast<size_t>(materialIndex) < materials.size())) {
				VK_CORE_CRITICAL("AssignMaterial: materialIndex must be less than m_Materials.size()");
			}

			Material& material = submesh.material;

			if (materialIndex != -1) {
				material = materials[materialIndex];
			}

			// create material descriptor
			//material.m_materialDescriptor = std::make_shared<MaterialDescriptor>(material.m_materialTextures);
		}

		VK_CORE_INFO("material assigned (Assimp): material index {0}", materialIndex);
	}
}  // namespace RVK
