#pragma once

#include "Framework/Vulkan/VKUtils.h"

namespace RVK {
	class RVKDescriptorSetLayout {
	public:
		class Builder {
		public:
			Builder() = default;

			Builder& AddBinding(
				u32 binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				u32 count = 1);
			std::unique_ptr<RVKDescriptorSetLayout> Build() const;

		private:
			std::unordered_map<u32, VkDescriptorSetLayoutBinding> m_bindings{};
		};

		RVKDescriptorSetLayout(std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings);
		~RVKDescriptorSetLayout();

		NO_COPY(RVKDescriptorSetLayout)

		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

	private:
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::unordered_map<u32, VkDescriptorSetLayoutBinding> m_bindings;

		friend class RVKDescriptorWriter;
	};

	class RVKDescriptorPool {
	public:
		class Builder {
		public:
			Builder() = default;

			Builder& AddPoolSize(VkDescriptorType descriptorType, u32 count);
			Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& SetMaxSets(u32 count);
			std::unique_ptr<RVKDescriptorPool> Build() const;

		private:
			std::vector<VkDescriptorPoolSize> m_poolSizes{};
			u32 m_maxSets = 1000;
			VkDescriptorPoolCreateFlags m_poolFlags = 0;
		};

		RVKDescriptorPool(
			u32 maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~RVKDescriptorPool();

		NO_COPY(RVKDescriptorPool)

		bool AllocateDescriptor(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

		void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

		VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }

		void ResetPool();

	private:
		VkDescriptorPool m_descriptorPool;

		friend class RVKDescriptorWriter;
	};

	class RVKDescriptorWriter {
	public:
		RVKDescriptorWriter(RVKDescriptorSetLayout& setLayout, RVKDescriptorPool& pool);

		RVKDescriptorWriter& WriteBuffer(u32 binding, VkDescriptorBufferInfo* bufferInfo);
		RVKDescriptorWriter& WriteImage(u32 binding, VkDescriptorImageInfo* imageInfo);

		bool Build(VkDescriptorSet& set);
		void Overwrite(VkDescriptorSet& set);

	private:
		RVKDescriptorSetLayout& m_setLayout;
		RVKDescriptorPool& m_pool;
		std::vector<VkWriteDescriptorSet> m_writes;
	};
}  // namespace RVK
