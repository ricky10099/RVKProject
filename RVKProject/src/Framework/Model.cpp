#include "Framework/Model.h"
#include "Framework/Vulkan/RVKDevice.h"

#include "ufbx/ufbx.c"

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
	Model::Model(const Model::TinyObjBuilder& builder) {
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	Model::Model(const Model::ufbxBuilder& builder) {
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	Model::Model(const Model::AssimpBuilder& builder) {
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	Model::~Model() {}

	std::unique_ptr<Model> Model::CreateModelFromFile(const std::string& filepath, Model::ModelType type) {
		switch (type)
		{
		case Model::ModelType::TinyObj: {
			TinyObjBuilder builder{};
			builder.LoadModel(ENGINE_DIR + filepath);
			return std::make_unique<Model>(builder);
		}
		case Model::ModelType::Ufbx: {
			ufbxBuilder builder{};
			builder.LoadModel(ENGINE_DIR + filepath);
			return std::make_unique<Model>(builder);
		}
		default: {
			AssimpBuilder builder{};
			builder.LoadModel(ENGINE_DIR + filepath);
			return std::make_unique<Model>(builder);
		}
			break;
		}

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

	void Model::TinyObjBuilder::LoadModel(const std::string& filepath) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
			VK_CORE_CRITICAL(warn + err);
		}

		vertices.clear();
		indices.clear();

		VK_CORE_TRACE("Tiny Obj Model Loaded");
		int count = 0;
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
				++count;
				indices.push_back(uniqueVertices[vertex]);

				VK_CORE_TRACE("Vertex position: {0}, {1}, {2} Vertex normal: {3}, {4}, {5}", vertex.position.x, vertex.position.y, vertex.position.z, vertex.normal.x, vertex.normal.y, vertex.normal.z);
				VK_CORE_TRACE("Index: {0}", uniqueVertices[vertex]);
			}
		}

		VK_CORE_TRACE("Loop count: {0}", count);
	}

	void Model::ufbxBuilder::LoadModel(const std::string& filepath) {
		ufbx_load_opts loadOptions{};
		loadOptions.load_external_files = true;
		loadOptions.ignore_missing_external_files = true;
		loadOptions.generate_missing_normals = true;
		loadOptions.target_axes = {
			.right = UFBX_COORDINATE_AXIS_POSITIVE_X,
			.up = UFBX_COORDINATE_AXIS_POSITIVE_Y,
			.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z,
		};
		loadOptions.target_unit_meters = 1.0f;

		ufbx_error ufbxError;
		ufbx_scene* scene = ufbx_load_file(filepath.c_str(), &loadOptions, &ufbxError);

		if (!scene) {
			char errorBuffer[512];
			ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
			VK_CORE_CRITICAL(errorBuffer);
		}
		VK_CORE_TRACE("ufbx Model Loaded");

		vertices.clear();
		indices.clear();
		std::unordered_map<Vertex, u32> uniqueVertices{};
		int count = 0;
		// Let's just list all objects within the scene for example:
		for (size_t i = 0; i < scene->nodes.count; i++) {
			ufbx_node* node = scene->nodes.data[i];
			if (node->is_root) continue;

			VK_CORE_TRACE("Object: {0}\n", node->name.data);
			if (node->mesh) {
				VK_CORE_TRACE("-> mesh with {0} faces\n", node->mesh->faces.count);
			}

			for(size_t j = 0; j < node->mesh->num_triangles; j++) {
				auto face = node->mesh->faces.data[j];
				// Loop through the corners of the polygon.
				for (uint32_t corner = 0; corner < face.num_indices; corner++) {

					// Faces are defined by consecutive indices, one for each corner.
					uint32_t index = face.index_begin + corner;

					// Retrieve the position, normal and uv for the vertex.
					ufbx_vec3 position = ufbx_get_vertex_vec3(&node->mesh->vertex_position, index);
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&node->mesh->vertex_normal, index);
					ufbx_vec2 uv{0.0f, 0.0f};
					if (node->mesh->vertex_uv.exists) {
						uv = ufbx_get_vertex_vec2(&node->mesh->vertex_uv, index);
					}
					Vertex vertex{};

					vertex.position = { position.x, position.y, position.z };
					vertex.normal = { normal.x, normal.y, normal.z };
					vertex.uv = { uv.x, uv.y };
					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<u32>(vertices.size());
						vertices.push_back(vertex);
					}
					indices.push_back(uniqueVertices[vertex]);
					VK_CORE_TRACE("Vertex position: {0}, {1}, {2} Vertex normal: {3}, {4}, {5}", vertex.position.x, vertex.position.y, vertex.position.z, vertex.normal.x, vertex.normal.y, vertex.normal.z);
					VK_CORE_TRACE("Index: {0}", uniqueVertices[vertex]);
					++count;
				}
			}
		}
		VK_CORE_TRACE("Loop count: {0}", count);
		ufbx_free_scene(scene);
	}

	void Model::ufbxBuilder::convert_mesh_part(ufbx_mesh* mesh, ufbx_mesh_part* part)
	{
		vertices.clear();
		std::vector<uint32_t> tri_indices;
		tri_indices.resize(mesh->max_face_triangles * 3);

		// Iterate over each face using the specific material.
		for (uint32_t face_index : part->face_indices) {
			ufbx_face face = mesh->faces[face_index];

			// Triangulate the face into `tri_indices[]`.
			uint32_t num_tris = ufbx_triangulate_face(
				tri_indices.data(), tri_indices.size(), mesh, face);

			// Iterate over each triangle corner contiguously.
			for (size_t i = 0; i < num_tris * 3; i++) {
				uint32_t index = tri_indices[i];

				Vertex v;
				if(mesh->vertex_position.exists)
				v.position = { mesh->vertex_position.values.data[index].x, mesh->vertex_position.values.data[index].y, mesh->vertex_position.values.data[index].z };
				if (mesh->vertex_color.exists) {
					v.color = { mesh->vertex_color.values.data[index].x, mesh->vertex_color.values.data[index].y, mesh->vertex_color.values.data[index].z };
				}
				else {
					v.color = { 1.0f, 1.0f, 1.0f };
				}
				if (mesh->vertex_normal.exists)
				v.normal = { mesh->vertex_normal.values.data[index].x, mesh->vertex_normal.values.data[index].y, mesh->vertex_normal.values.data[index].z };
				if (mesh->vertex_uv.exists) {
					v.uv = { mesh->vertex_uv.values.data[index].x, mesh->vertex_uv.values.data[index].y };
				}
				vertices.push_back(v);
			}
		}

		// Should have written all the vertices.
		assert(vertices.size() == part->num_triangles * 3);

		// Generate the index buffer.
		ufbx_vertex_stream streams[1] = {
			{ vertices.data(), vertices.size(), sizeof(Vertex) },
		};
		indices.clear();
		indices.resize(part->num_triangles * 3);

		// This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
		// indices are written in `indices[]` and the number of unique vertices is returned.
		size_t num_vertices = ufbx_generate_indices(
			streams, 1, indices.data(), indices.size(), nullptr, nullptr);

		// Trim to only unique vertices.
		vertices.resize(num_vertices);
	}

	void Model::ufbxBuilder::LoadVertexData(const ufbx_node* fbxNodePtr, u32 const submeshIndex)
	{
		ufbx_mesh& fbxMesh = *fbxNodePtr->mesh; // mesh for this node, contains submeshes
		const ufbx_mesh_part& fbxSubmesh = fbxNodePtr->mesh->material_parts[submeshIndex];
		size_t numFaces = fbxSubmesh.num_faces;

		//if (!(fbxSubmesh.num_triangles))
		//{
		//	LOG_CORE_CRITICAL("UFbxBuilder::LoadVertexData: only triangle meshes are supported");
		//	return;
		//}

		size_t numVerticesBefore = vertices.size();
		size_t numIndicesBefore = indices.size();

		//Submesh& submesh = m_Submeshes[submeshIndex];
		//submesh.m_FirstVertex = numVerticesBefore;
		//submesh.m_FirstIndex = numIndicesBefore;
		//// submesh.m_VertexCount = 0;
		//submesh.m_IndexCount = 0;
		//submesh.m_InstanceCount = m_InstanceCount;

		glm::vec4 diffuseColor{1.0f};
		{
			//ufbx_material_map& baseColorMap = fbxNodePtr->materials[submeshIndex]->pbr.base_color;
			//diffuseColor = baseColorMap.has_value ? glm::vec4(baseColorMap.value_vec4.x, baseColorMap.value_vec4.y,
			//	baseColorMap.value_vec4.z, baseColorMap.value_vec4.w)
			//	: glm::vec4(1.0f);
		}

		{ // vertices
			bool hasTangents = fbxMesh.vertex_tangent.exists;
			bool hasUVs = fbxMesh.uv_sets.count;
			bool hasVertexColors = fbxMesh.vertex_color.exists;
			ufbx_skin_deformer* fbxSkin = nullptr;
			if (fbxMesh.skin_deformers.count)
			{
				fbxSkin = fbxMesh.skin_deformers.data[0];
			}

			//m_FbxNoBuiltInTangents = m_FbxNoBuiltInTangents || (!hasTangents);
			for (size_t fbxFaceIndex = 0; fbxFaceIndex < numFaces; ++fbxFaceIndex)
			{
				ufbx_face& fbxFace = fbxMesh.faces[fbxSubmesh.face_indices.data[fbxFaceIndex]];
				size_t numTriangleIndices = fbxMesh.max_face_triangles * 3;
				std::vector<u32> verticesPerFaceIndexBuffer(numTriangleIndices);
				size_t numTriangles =
					ufbx_triangulate_face(verticesPerFaceIndexBuffer.data(), numTriangleIndices, &fbxMesh, fbxFace);
				size_t numVerticesPerFace = 3 * numTriangles;
				for (u32 vertexPerFace = 0; vertexPerFace < numVerticesPerFace; ++vertexPerFace)
				{
					// if the face is a quad, then 2 triangles, numVerticesPerFace = 6
					u32 vertexPerFaceIndex = verticesPerFaceIndexBuffer[vertexPerFace];

					Vertex vertex{};

					// position
					u32 fbxVertexIndex = fbxMesh.vertex_indices[vertexPerFaceIndex];
					{
						ufbx_vec3& positionFbx = fbxMesh.vertices[fbxVertexIndex];
						vertex.position = glm::vec3(positionFbx.x, positionFbx.y, positionFbx.z);
					}

					// normals, always defined if `ufbx_load_opts.generate_missing_normals` is used
					{
						u32 fbxNormalIndex = fbxMesh.vertex_normal.indices[vertexPerFaceIndex];
						/*CORE_ASSERT(fbxNormalIndex < fbxMesh.vertex_normal.values.count,
							"LoadVertexData: memory violation normals");*/
						ufbx_vec3& normalFbx = fbxMesh.vertex_normal.values.data[fbxNormalIndex];
						vertex.normal = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);
					}
					//if (hasTangents) // tangents (check `tangent space` in Blender when exporting fbx)
					//{
					//	u32 fbxTangentIndex = fbxMesh.vertex_tangent.indices[vertexPerFaceIndex];
					//	/*CORE_ASSERT(fbxTangentIndex < fbxMesh.vertex_tangent.values.count,
					//		"LoadVertexData: memory violation tangents");*/
					//	ufbx_vec3& tangentFbx = fbxMesh.vertex_tangent.values.data[fbxTangentIndex];
					//	vertex.m_Tangent = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
					//}

					if (hasUVs) // uv coordinates
					{
						u32 fbxUVIndex = fbxMesh.vertex_uv.indices[vertexPerFaceIndex];
						/*CORE_ASSERT(fbxUVIndex < fbxMesh.vertex_uv.values.count,
							"LoadVertexData: memory violation uv coordinates");*/
						ufbx_vec2& uvFbx = fbxMesh.vertex_uv.values.data[fbxUVIndex];
						vertex.uv = glm::vec2(uvFbx.x, uvFbx.y);
					}

					if (hasVertexColors) // vertex colors
					{
						u32 fbxColorIndex = fbxMesh.vertex_color.indices[vertexPerFaceIndex];
						ufbx_vec4& colorFbx = fbxMesh.vertex_color.values.data[fbxColorIndex];

						// convert from sRGB to linear
						glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.x, colorFbx.y, colorFbx.z), glm::vec3(2.2f));
						glm::vec4 vertexColor(linearColor.x, linearColor.y, linearColor.z, colorFbx.w);
						vertex.color = vertexColor * diffuseColor;
					}
					else
					{
						vertex.color = diffuseColor;
					}
					//if (fbxSkin)
					//{
					//	ufbx_skin_vertex skinVertex = fbxSkin->vertices[fbxVertexIndex];
					//	size_t numWeights =
					//		skinVertex.num_weights < MAX_JOINT_INFLUENCE ? skinVertex.num_weights : MAX_JOINT_INFLUENCE;

					//	for (size_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
					//	{
					//		ufbx_skin_weight skinWeight = fbxSkin->weights.data[skinVertex.weight_begin + weightIndex];
					//		int jointIndex = skinWeight.cluster_index;
					//		float weight = skinWeight.weight;

					//		switch (weightIndex)
					//		{
					//		case 0:
					//			vertex.m_JointIds.x = jointIndex;
					//			vertex.m_Weights.x = weight;
					//			break;
					//		case 1:
					//			vertex.m_JointIds.y = jointIndex;
					//			vertex.m_Weights.y = weight;
					//			break;
					//		case 2:
					//			vertex.m_JointIds.z = jointIndex;
					//			vertex.m_Weights.z = weight;
					//			break;
					//		case 3:
					//			vertex.m_JointIds.w = jointIndex;
					//			vertex.m_Weights.w = weight;
					//			break;
					//		default:
					//			break;
					//		}
					//	}
					//	{ // normalize weights
					//		float weightSum =
					//			vertex.m_Weights.x + vertex.m_Weights.y + vertex.m_Weights.z + vertex.m_Weights.w;
					//		if (weightSum > std::numeric_limits<float>::epsilon())
					//		{
					//			vertex.m_Weights = vertex.m_Weights / weightSum;
					//		}
					//	}
					//}
					vertices.push_back(vertex);
				}
			}
		}

		// resolve indices
		// A face has four vertices, while above loop generates at least six vertices for per face)
		{
			// get number of all vertices created from above (faces * trianglesPerFace * 3)
			u32 submeshAllVertices = vertices.size() - numVerticesBefore;

			// create a ufbx vertex stream with data pointing to the first vertex of this submesh
			// (m_vertices is for all submeshes)
			ufbx_vertex_stream streams;
			streams.data = &vertices[numVerticesBefore];
			streams.vertex_count = submeshAllVertices;
			streams.vertex_size = sizeof(Vertex);

			// index buffer: add space for all new vertices from above
			indices.resize(numIndicesBefore + submeshAllVertices);

			// ufbx_generate_indices() will rearrange m_Vertices (via streams.data) and fill m_Indices
			ufbx_error ufbxError;
			size_t numVertices = ufbx_generate_indices(&streams, 1 /*size_t num_streams*/, &indices[numIndicesBefore],
				submeshAllVertices, nullptr, &ufbxError);

			// handle error
			if (ufbxError.type != UFBX_ERROR_NONE)
			{
				char errorBuffer[512];
				ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
				//LOG_CORE_CRITICAL("UFbxBuilder: creation of index buffer failed, file: {0}, error: {1},  node: {2}",
				//	m_Filepath, errorBuffer, fbxNodePtr->name.data);
			}

			// m_Vertices can be downsized now
			vertices.resize(numVerticesBefore + numVertices);
			//submesh.m_VertexCount = numVertices;
			//submesh.m_IndexCount = submeshAllVertices;
		}
	}

	void Model::AssimpBuilder::LoadModel(const std::string& filepath) {
		// Assimp importer
		Assimp::Importer importer;

		// Load the model file
		m_scene = importer.ReadFile(filepath,
			aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
			aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

		// Check if the scene or root node is null
		if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode) {
			throw std::runtime_error("Failed to load model: " + filepath);
		}
		//VK_CORE_TRACE("assimp Model Loaded");

		// Process Assimp's root node
		ProcessNode(m_scene->mRootNode, m_scene);
	}

	void Model::AssimpBuilder::ProcessNode(aiNode* node, const aiScene* scene) {
		if (node->mNumMeshes){
			LoadVertexData(node);
		}
		u32 childNodeCount = node->mNumChildren;
		for (u32 childNodeIndex = 0; childNodeIndex < childNodeCount; ++childNodeIndex)
		{
			ProcessNode(node->mChildren[childNodeIndex], scene);
		}
	}

	void Model::AssimpBuilder::CreateObject(const aiNode* node) {
		
	}

	void Model::AssimpBuilder::LoadVertexData(const aiNode* fbxNodePtr, int vertexColorSet, u32 uvSet)
	{
		vertices.clear();
		indices.clear();

		u32 numMeshes = fbxNodePtr->mNumMeshes;
		if (numMeshes)
		{
			for (u32 meshIndex = 0; meshIndex < numMeshes; ++meshIndex)
			{
				LoadVertexData(fbxNodePtr, meshIndex, fbxNodePtr->mMeshes[meshIndex], vertexColorSet, uvSet);
			}
		}
	}

	void Model::AssimpBuilder::LoadVertexData(const aiNode* fbxNodePtr, u32 const meshIndex, u32 const fbxMeshIndex,
		int vertexColorSet, u32 uvSet)
	{
		const aiMesh* mesh = m_scene->mMeshes[fbxMeshIndex];

		// only triangle mesh supported
		if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
		{
			//LOG_CORE_CRITICAL("FbxBuilder::LoadVertexData: only triangle meshes are supported");
			return;
		}

		const u32 numVertices = mesh->mNumVertices;
		const u32 numFaces = mesh->mNumFaces;
		const u32 numIndices = numFaces * 3; // 3 indices per triangle a.k.a face

		size_t numVerticesBefore = vertices.size();
		size_t numIndicesBefore = indices.size();
		vertices.resize(numVerticesBefore + numVertices);
		indices.resize(numIndicesBefore + numIndices);

		{ // vertices
			bool hasPositions = mesh->HasPositions();
			bool hasNormals = mesh->HasNormals();
			bool hasTangents = mesh->HasTangentsAndBitangents();
			bool hasUVs = mesh->HasTextureCoords(uvSet);
			bool hasColors = mesh->HasVertexColors(vertexColorSet);

			//CORE_ASSERT(hasPositions, "no postions found in " + m_Filepath);
			//CORE_ASSERT(hasNormals, "no normals found in " + m_Filepath);

			m_FbxNoBuiltInTangents = m_FbxNoBuiltInTangents || (!hasTangents);

			u32 vertexIndex = numVerticesBefore;
			for (u32 fbxVertexIndex = 0; fbxVertexIndex < numVertices; ++fbxVertexIndex)
			{
				Vertex& vertex = vertices[vertexIndex];

				if (hasPositions)
				{ // position (guaranteed to always be there)
					aiVector3D& positionFbx = mesh->mVertices[fbxVertexIndex];
					vertex.position = glm::vec3(positionFbx.x, positionFbx.y, positionFbx.z);
				}

				if (hasNormals) // normals
				{
					aiVector3D& normalFbx = mesh->mNormals[fbxVertexIndex];
					vertex.normal = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);
				}

				//if (hasTangents) // tangents
				//{
				//	aiVector3D& tangentFbx = mesh->mTangents[fbxVertexIndex];
				//	vertex.tangent = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
				//}

				if (hasUVs) // uv coordinates
				{
					aiVector3D& uvFbx = mesh->mTextureCoords[uvSet][fbxVertexIndex];
					vertex.uv = glm::vec2(uvFbx.x, uvFbx.y);
				}

				// vertex colors
				{
					glm::vec4 vertexColor;
					u32 materialIndex = mesh->mMaterialIndex;
					if (hasColors)
					{
						aiColor4D& colorFbx = mesh->mColors[vertexColorSet][fbxVertexIndex];
						glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.r, colorFbx.g, colorFbx.b), glm::vec3(2.2f));
						vertexColor = glm::vec4(linearColor.r, linearColor.g, linearColor.b, colorFbx.a);
						//vertex.color = vertexColor * m_Materials[materialIndex].m_PbrMaterial.m_DiffuseColor;
					}
					else
					{
						vertex.color = { 1.0f, 1.0f, 1.0f };
					}
				}
				++vertexIndex;
			}
		}

		// Indices
		{
			u32 index = numIndicesBefore;
			for (u32 faceIndex = 0; faceIndex < numFaces; ++faceIndex)
			{
				const aiFace& face = mesh->mFaces[faceIndex];
				indices[index + 0] = face.mIndices[0];
				indices[index + 1] = face.mIndices[1];
				indices[index + 2] = face.mIndices[2];
				index += 3;
			}
		}

		//// bone indices and bone weights
		//{
		//	u32 numberOfBones = mesh->mNumBones;
		//	std::vector<u32> numberOfBonesBoundtoVertex;
		//	numberOfBonesBoundtoVertex.resize(vertices.size(), 0);
		//	for (u32 boneIndex = 0; boneIndex < numberOfBones; ++boneIndex)
		//	{
		//		aiBone& bone = *mesh->mBones[boneIndex];
		//		u32 numberOfWeights = bone.mNumWeights;

		//		// loop over vertices that are bound to that bone
		//		for (u32 weightIndex = 0; weightIndex < numberOfWeights; ++weightIndex)
		//		{
		//			u32 vertexId = bone.mWeights[weightIndex].mVertexId;
		//			CORE_ASSERT(vertexId < vertices.size(), "memory violation");
		//			float weight = bone.mWeights[weightIndex].mWeight;
		//			switch (numberOfBonesBoundtoVertex[vertexId])
		//			{
		//			case 0:
		//				vertices[vertexId].m_JointIds.x = boneIndex;
		//				vertices[vertexId].m_Weights.x = weight;
		//				break;
		//			case 1:
		//				vertices[vertexId].m_JointIds.y = boneIndex;
		//				vertices[vertexId].m_Weights.y = weight;
		//				break;
		//			case 2:
		//				vertices[vertexId].m_JointIds.z = boneIndex;
		//				vertices[vertexId].m_Weights.z = weight;
		//				break;
		//			case 3:
		//				vertices[vertexId].m_JointIds.w = boneIndex;
		//				vertices[vertexId].m_Weights.w = weight;
		//				break;
		//			default:
		//				break;
		//			}
		//			// track how many times this bone was hit
		//			// (up to four bones can be bound to a vertex)
		//			++numberOfBonesBoundtoVertex[vertexId];
		//		}
		//	}
		//	// normalize weights
		//	for (u32 vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
		//	{
		//		glm::vec4& boneWeights = vertices[vertexIndex].m_Weights;
		//		float weightSum = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
		//		if (weightSum > std::numeric_limits<float>::epsilon())
		//		{
		//			vertices[vertexIndex].m_Weights = glm::vec4(boneWeights.x / weightSum, boneWeights.y / weightSum,
		//				boneWeights.z / weightSum, boneWeights.w / weightSum);
		//		}
		//	}
		//}
	}

	void Model::AssimpBuilder::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
		vertices.clear();
		indices.clear();
		std::unordered_map<Vertex, u32> uniqueVertices{};

		// Extract vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			Vertex vertex;

			// Position
			vertex.position = {
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z
			};

			// Normal
			vertex.normal = /*-1.0f * glm::vec3*/{
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			};

			// Texture Coordinates
			//if (mesh->mTextureCoords[0]) { // Check if there are texture coordinates
			//	vertex.uv = {
			//		mesh->mTextureCoords[0][i].x,
			//		mesh->mTextureCoords[0][i].y
			//	};
			//}
			//else {
				vertex.uv = { 0.0f, 0.0f };
			//}
			//vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<u32>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);

			//VK_CORE_TRACE("Vertex: {0}, {1}, {2}", vertex.position.x, vertex.position.y, vertex.position.z);
			//VK_CORE_TRACE("Index: {0}", uniqueVertices[vertex]);

		}

		//// Extract indices
		//for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		//	aiFace face = mesh->mFaces[i];

		//	for (unsigned int j = 0; j < face.mNumIndices; ++j) {
		//		indices.push_back(face.mIndices[j]);
		//		VK_CORE_TRACE("Index: {0}", indices[j]);
		//	}
		//}
	}
}  // namespace RVK
