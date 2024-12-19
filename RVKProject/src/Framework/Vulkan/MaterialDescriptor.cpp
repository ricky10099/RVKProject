#include "Framework/Vulkan/MaterialDescriptor.h"
#include "Framework/Vulkan/RVKDescriptors.h"
#include "Framework/RVKApp.h"

namespace RVK {
	extern std::shared_ptr<Texture> DefaultTexture;
	MaterialDescriptor::MaterialDescriptor(Material& material, Material::MaterialTextures& textures) {
		material.m_materialBuffer = std::make_shared<RVKBuffer>(
			sizeof(Material::PBRMaterial),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			RVKDevice::s_rvkDevice->m_properties.limits.minUniformBufferOffsetAlignment
		);
		material.m_materialBuffer->Map();

		// textures
		std::shared_ptr<Texture> diffuseMap;
		std::shared_ptr<Texture> normalMap;
		std::shared_ptr<Texture> roughnessMetallicMap;
		std::shared_ptr<Texture> emissiveMap;
		std::shared_ptr<Texture> roughnessMap;
		std::shared_ptr<Texture> metallicMap;
		std::shared_ptr<Texture> dummy = std::make_shared<Texture>();
		bool success = dummy->Init("../models/checker.png", Texture::USE_SRGB);

		diffuseMap = textures[Material::DIFFUSE_MAP_INDEX] ? textures[Material::DIFFUSE_MAP_INDEX] : dummy;
		normalMap = textures[Material::NORMAL_MAP_INDEX] ? textures[Material::NORMAL_MAP_INDEX] : dummy;
		roughnessMetallicMap = textures[Material::ROUGHNESS_METALLIC_MAP_INDEX] ? textures[Material::ROUGHNESS_METALLIC_MAP_INDEX] : dummy;
		emissiveMap = textures[Material::EMISSIVE_MAP_INDEX] ? textures[Material::EMISSIVE_MAP_INDEX] : dummy;
		roughnessMap = textures[Material::ROUGHNESS_MAP_INDEX] ? textures[Material::ROUGHNESS_MAP_INDEX] : dummy;
		metallicMap = textures[Material::METALLIC_MAP_INDEX] ? textures[Material::METALLIC_MAP_INDEX] : dummy;

		{
			RVKDescriptorSetLayout::Builder builder{};
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
			builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
			//.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			//.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
			std::unique_ptr<RVKDescriptorSetLayout> localDescriptorSetLayout = builder.Build();

			auto bufferInfo = material.m_materialBuffer->DescriptorInfo();
			auto& imageInfo0 = static_cast<Texture*>(diffuseMap.get())->GetDescriptorImageInfo();
			//auto& imageInfo1 = static_cast<Texture*>(normalMap.get())->GetDescriptorImageInfo();
			//auto& imageInfo2 = static_cast<Texture*>(roughnessMetallicMap.get())->GetDescriptorImageInfo();
			//auto& imageInfo3 = static_cast<Texture*>(emissiveMap.get())->GetDescriptorImageInfo();
			//auto& imageInfo4 = static_cast<Texture*>(roughnessMap.get())->GetDescriptorImageInfo();
			//auto& imageInfo5 = static_cast<Texture*>(metallicMap.get())->GetDescriptorImageInfo();

			RVKDescriptorWriter descriptorWriter(*localDescriptorSetLayout, *GetApp().globalPool);
			descriptorWriter.WriteBuffer(0, &bufferInfo);
			descriptorWriter.WriteImage(1, &imageInfo0);
			//.WriteImage(1, &imageInfo1)
			//.WriteImage(2, &imageInfo2)
			//.WriteImage(3, &imageInfo3)
			//.WriteImage(4, &imageInfo4)
			//.WriteImage(5, &imageInfo5);
			descriptorWriter.Build(m_descriptorSet);
		}
	}

	MaterialDescriptor::MaterialDescriptor(MaterialDescriptor const& other) {
		m_descriptorSet = other.m_descriptorSet;
	}
}// namespace RVK