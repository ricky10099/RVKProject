#include "Framework/Model.h"
#include "Framework/Vulkan/RVKDevice.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>



namespace std {
	template <>
	struct hash<RVK::Model::Vertex> {
		size_t operator()(RVK::Model::Vertex const& vertex) const {
			size_t seed = 0;
			HashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}  // namespace std

namespace RVK {
	Model::Model(const Model::Builder& builder) {
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	Model::~Model() {}

	std::unique_ptr<Model> Model::CreateModelFromFile(const std::string& filepath) {
		Builder builder{};
		builder.LoadModel(ENGINE_DIR + filepath);
		return std::make_unique<Model>(builder);
	}

	void Model::CreateVertexBuffers(const std::vector<Vertex>& vertices) {
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

	void Model::CreateIndexBuffers(const std::vector<u32>& indices) {
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

	void Model::Draw(VkCommandBuffer commandBuffer) {
		if (m_hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
		}
	}

	void Model::Bind(VkCommandBuffer commandBuffer) {
		VkBuffer buffers[] = { m_vertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (m_hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	std::vector<VkVertexInputBindingDescription> Model::Vertex::GetBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Model::Vertex::GetAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

		return attributeDescriptions;
	}

	void Model::AssimpLoader::LoadModel(std::string_view filepath) {
		Assimp::Importer importer;

		const auto aiscene = importer.ReadFile(filepath.data(), aiProcess_Triangulate);
		if (!aiscene) {
			VK_CORE_CRITICAL("Unable to load: {}: {}", filepath, importer.GetErrorString());
			return;
		}

		std::vector<std::shared_ptr<Material>> materials;
		//if (loadMaterials) {
		materials.reserve(aiscene->mNumMaterials);
		LoadMaterials(aiscene);

		vertices.clear();
		indices.clear();
		int num = aiscene->mRootNode.mNumVertices;
		for (size_t i = 0; i < aiscene->mRootNode->mNumMeshes; ++i) {
			vertices.resize();
			for (auto i = 0u; i < aimesh.mNumVertices; ++i) {
				const auto& position = aimesh.mVertices[i];
				const auto& normal = aimesh.mNormals[i];

				auto& vertex = result->vertices.emplace_back();
				vertex.position = { position.x, position.y, position.z };
				vertex.normal = { normal.x, normal.y, normal.z };
				vertex.matIndex = aimesh.mMaterialIndex;
			}

			result->indices.reserve((size_t)aimesh.mNumFaces * 3);
			for (auto i = 0u; i < aimesh.mNumFaces; ++i) {
				const auto& face = aimesh.mFaces[i];

				if (face.mNumIndices != 3) {
					RAYGUN_WARN("Face {} of mesh {} has {} vertices, skipping", i, aimesh.mName.C_Str(), face.mNumIndices);
					continue;
				}

				result->indices.push_back(face.mIndices[0]);
				result->indices.push_back(face.mIndices[1]);
				result->indices.push_back(face.mIndices[2]);
			}

			RAYGUN_DEBUG("Loaded Mesh: {}: {} vertices", aimesh.mName.C_Str(), result->vertices.size());

			return result;
		}
		//MarkNode(aiscene->mRootNode, aiscene);
		//ProcessNode(aiscene->mRootNode);
		

		//for (auto i = 0u; i < aiscene->mNumMaterials; ++i) {
		//	const auto aimaterial = aiscene->mMaterials[i];

		//	aiString matName;
		//	aimaterial->Get(AI_MATKEY_NAME, matName);
		//	materials.push_back(LoadMaterial(matName.C_Str()));
		//}
		////}

		//for (auto i = 0u; i < aiscene->mRootNode->mNumChildren; ++i) {
		//	const auto ainode = aiscene->mRootNode->mChildren[i];

		//	auto childModel = std::make_shared<render::Model>();
		//	childModel->mesh = collapseMeshes(aiscene, ainode);
		//	childModel->materials = materials;

		//	RG().resourceManager().registerModel(childModel);

		//	auto child = emplaceChild(ainode->mName.C_Str());
		//	child->setTransform(utils::toTransform(ainode->mTransformation));
		//	child->model = childModel;
		//}
	}

	void Model::AssimpLoader::LoadMaterials(const aiScene* scene){
		u32 numMaterials = scene->mNumMaterials;
		m_materials.resize(numMaterials);
		//m_MaterialTextures.resize(numMaterials);
		for (auto i = 0u; i < numMaterials; ++i){
			const aiMaterial* fbxMaterial = scene->mMaterials[i];
			// PrintMaps(fbxMaterial);

			Material& material = m_materials[i];

			LoadProperties(fbxMaterial, material.m_PBRMaterial);

			LoadMap(fbxMaterial, aiTextureType_DIFFUSE, i);
			LoadMap(fbxMaterial, aiTextureType_NORMALS, i);
			LoadMap(fbxMaterial, aiTextureType_SHININESS, i);
			LoadMap(fbxMaterial, aiTextureType_METALNESS, i);
			LoadMap(fbxMaterial, aiTextureType_EMISSIVE, i);
		}
	}

	void Model::AssimpLoader::LoadProperties(const aiMaterial* fbxMaterial, Material::PBRMaterial& pbrMaterial){
		// diffuse
		{
			aiColor3D diffuseColor;
			if (fbxMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS){
				pbrMaterial.diffuseColor.r = diffuseColor.r;
				pbrMaterial.diffuseColor.g = diffuseColor.g;
				pbrMaterial.diffuseColor.b = diffuseColor.b;
			}
		}

		// roughness
		{
			float roughnessFactor;
			if (fbxMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == aiReturn_SUCCESS){
				pbrMaterial.roughness = roughnessFactor;
			}
			else{
				pbrMaterial.roughness = 0.1f;
			}
		}

		// metallic
		{
			float metallicFactor;
			if (fbxMaterial->Get(AI_MATKEY_REFLECTIVITY, metallicFactor) == aiReturn_SUCCESS){
				pbrMaterial.metallic = metallicFactor;
			}
			else if (fbxMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == aiReturn_SUCCESS){
				pbrMaterial.metallic = metallicFactor;
			}
			else{
				pbrMaterial.metallic = 0.886f;
			}
		}

		// emissive color
		{
			aiColor3D emission;
			auto result = fbxMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emission);
			if (result == aiReturn_SUCCESS){
				pbrMaterial.emissiveColor = glm::vec3(emission.r, emission.g, emission.b);
			}
		}

		// emissive strength
		{ 
			float emissiveStrength;
			auto result = fbxMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength);
			if (result == aiReturn_SUCCESS){
				pbrMaterial.emissiveStrength = emissiveStrength;
			}
		}

		pbrMaterial.normalMapIntensity = 1.0f;
	}

	void Model::AssimpLoader::LoadMap(const aiMaterial* fbxMaterial, aiTextureType textureType, int materialIndex){
		u32 textureCount = fbxMaterial->GetTextureCount(textureType);
		if (!textureCount){
			return;
		}

		Material& material = m_materials[materialIndex];
		Material::PBRMaterial& pbrMaterial = material.m_PBRMaterial;
		Material::MaterialTextures& materialTextures = m_materialTextures[materialIndex];

		aiString aiFilepath;
		auto getTexture = fbxMaterial->GetTexture(textureType, 0 /* first map*/, &aiFilepath);
		std::string fbxFilepath(aiFilepath.C_Str());
		std::string filepath(fbxFilepath);
		if (getTexture == aiReturn_SUCCESS){
			switch (textureType){
				// LoadTexture is inside switch statement for sRGB and UNORM
			case aiTextureType_DIFFUSE:{
				auto texture = LoadTexture(filepath, Texture::USE_SRGB);
				if (texture){
					materialTextures[Material::DIFFUSE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_DIFFUSE_MAP;
				}
				break;
			}
			case aiTextureType_NORMALS:{
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture){
					materialTextures[Material::NORMAL_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_NORMAL_MAP;
				}
				break;
			}
			case aiTextureType_SHININESS:{
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture){
					materialTextures[Material::ROUGHNESS_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_ROUGHNESS_MAP;
				}
				break;
			}
			case aiTextureType_METALNESS:{
				auto texture = LoadTexture(filepath, Texture::USE_UNORM);
				if (texture){
					materialTextures[Material::METALLIC_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_METALLIC_MAP;
				}
				break;
			}
			case aiTextureType_EMISSIVE:{
				auto texture = LoadTexture(filepath, Texture::USE_SRGB);
				if (texture){
					materialTextures[Material::EMISSIVE_MAP_INDEX] = texture;
					pbrMaterial.features |= Material::HAS_EMISSIVE_MAP;
					pbrMaterial.emissiveColor = glm::vec3(1.0f);
				}
				break;
			}
			default:{
				VK_CORE_ASSERT(false, "texture type not recognized");
			}
			}
		}
	}

	std::shared_ptr<Texture> Model::AssimpLoader::LoadTexture(std::string const& filepath, bool useSRGB){
		std::shared_ptr<Texture> texture;
		bool loadSucess = false;

		//if (EngineCore::FileExists(filepath) && !EngineCore::IsDirectory(filepath))
		//{
		texture = std::make_shared<Texture>();
		loadSucess = texture->Init(filepath, useSRGB);

		if (!loadSucess) {
			VK_CORE_CRITICAL("bool AssimpLoader::LoadTexture(): file '{0}' not found", filepath);
		}
		//}
		//else if (EngineCore::FileExists(m_Basepath + filepath) && !EngineCore::IsDirectory(m_Basepath + filepath))
		//{
		//	texture = Texture::Create();
		//	loadSucess = texture->Init(m_Basepath + filepath, useSRGB);
		//}
		//else
		//{
		//	LOG_CORE_CRITICAL("bool FbxBuilder::LoadTexture(): file '{0}' not found", filepath);
		//}


		m_textures.push_back(texture);
		return texture;
	}

	//void Model::AssimpLoader::ProcessNode(const aiNode* fbxNodePtr)
	//{
	//	std::string nodeName = std::string(fbxNodePtr->mName.C_Str());
	//	//u32 currentNode = parentNode;

	//	if (m_HasMesh[hasMeshIndex])
	//	{
	//		if (fbxNodePtr->mNumMeshes)
	//		{
	//			currentNode = CreateGameObject(fbxNodePtr, parentNode);
	//		}
	//		else // one or more children have a mesh, but not this one --> create group node
	//		{
	//			// create game object and transform component
	//			auto entity = m_Registry.Create();
	//			{
	//				TransformComponent transform(LoadTransformationMatrix(fbxNodePtr));
	//				if (fbxNodePtr->mParent == m_FbxScene->mRootNode)
	//				{
	//					auto scale = transform.GetScale();
	//					transform.SetScale({ scale.x / 100.0f, scale.y / 100.0f, scale.z / 100.0f });
	//					auto translation = transform.GetTranslation();
	//					transform.SetTranslation({ translation.x / 100.0f, translation.y / 100.0f, translation.z / 100.0f });
	//				}
	//				m_Registry.emplace<TransformComponent>(entity, transform);
	//			}

	//			// create scene graph node and add to parent
	//			auto shortName = "::" + std::to_string(m_InstanceIndex) + "::" + nodeName;
	//			auto longName = m_Filepath + "::" + std::to_string(m_InstanceIndex) + "::" + nodeName;
	//			currentNode = m_SceneGraph.CreateNode(entity, shortName, longName, m_Dictionary);
	//			m_SceneGraph.GetNode(parentNode).AddChild(currentNode);
	//		}
	//	}
	//	++hasMeshIndex;

	//	uint childNodeCount = fbxNodePtr->mNumChildren;
	//	for (uint childNodeIndex = 0; childNodeIndex < childNodeCount; ++childNodeIndex)
	//	{
	//		ProcessNode(fbxNodePtr->mChildren[childNodeIndex], currentNode, hasMeshIndex);
	//	}
	//}

	//void Model::AssimpLoader::MarkNode(const aiNode* fbxNodePtr, const aiScene* scene){
	//	// each recursive call of this function marks a node in "m_HasMesh" if itself or a child has a mesh

	//	// does this Fbx node have a mesh?
	//	bool localHasMesh = false;

	//	// check if at least one mesh is usable, i.e. is a triangle mesh
	//	for (u32 i = 0; i < fbxNodePtr->mNumMeshes; ++i)
	//	{
	//		// retrieve index for global/scene mesh array
	//		u32 sceneMeshIndex = fbxNodePtr->mMeshes[i];
	//		aiMesh* mesh = scene->mMeshes[sceneMeshIndex];

	//		// check if triangle mesh
	//		if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)
	//		{
	//			localHasMesh = true;
	//			break;
	//		}
	//	}

	//	//int hasMeshIndex = m_HasMesh.size();
	//	//m_HasMesh.push_back(localHasMesh); // reserve space in m_HasMesh, so that ProcessNode can find it

	//	// do any of the child nodes have a mesh?
	//	u32 childNodeCount = fbxNodePtr->mNumChildren;
	//	for (u32 childNodeIndex = 0; childNodeIndex < childNodeCount; ++childNodeIndex)
	//	{
	//		/*bool childHasMesh = */
	//		MarkNode(fbxNodePtr->mChildren[childNodeIndex], scene);
	//		//localHasMesh = localHasMesh || childHasMesh;
	//	}
	//	//m_HasMesh[hasMeshIndex] = localHasMesh;
	//	//return localHasMesh;
	//}

	void Model::Builder::LoadModel(const std::string& filepath) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			VK_CORE_CRITICAL(warn + err);
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, u32> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<u32>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

}  // namespace RVK
