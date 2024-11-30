#include "Framework/Vulkan/RVKDescriptor.h"
#include "Framework/Vulkan/RVKDevice.h"

namespace RVK {
    // *************** Descriptor Set Layout Builder *********************
    RVKDescriptorSetLayout::Builder& RVKDescriptorSetLayout::Builder::AddBinding(
        u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, u32 count) {
        VK_CORE_ASSERT(m_bindings.count(binding) == 0, "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        m_bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<RVKDescriptorSetLayout> RVKDescriptorSetLayout::Builder::Build() const {
        return std::make_unique<RVKDescriptorSetLayout>(m_bindings);
    }

    // *************** Descriptor Set Layout *********************
    RVKDescriptorSetLayout::RVKDescriptorSetLayout(std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings)
        : m_bindings{ bindings } {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto const& it : bindings) {
            setLayoutBindings.push_back(it.second);
        }

        // Create Descriptor Set Layout with given bindings
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<u32>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        // Create Descriptor Set Layout
        VkResult result = vkCreateDescriptorSetLayout(RVKDevice::s_rvkDevice->GetLogicalDevice(),
            &descriptorSetLayoutInfo, nullptr, &m_descriptorSetLayout);
        VK_CHECK(result, "Failed to create a Descriptor Set Layout!");
    }

    RVKDescriptorSetLayout::~RVKDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool Builder *********************
    RVKDescriptorPool::Builder& RVKDescriptorPool::Builder::AddPoolSize(VkDescriptorType descriptorType, u32 count) {
        m_poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    RVKDescriptorPool::Builder& RVKDescriptorPool::Builder::SetPoolFlags(VkDescriptorPoolCreateFlags flags) {
        m_poolFlags = flags;
        return *this;
    }

    RVKDescriptorPool::Builder& RVKDescriptorPool::Builder::SetMaxSets(u32 count) {
        m_maxSets = count;
        return *this;
    }

    std::unique_ptr<RVKDescriptorPool> RVKDescriptorPool::Builder::Build() const {
        return std::make_unique<RVKDescriptorPool>(m_maxSets, m_poolFlags, m_poolSizes);
    }

    // *************** Descriptor Pool *********************
    RVKDescriptorPool::RVKDescriptorPool(u32 maxSets, VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes) {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.maxSets = maxSets;                                           // Maximun number of descriptor sets that can be created from pool
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());     // Amount of Pool Sizes being passed
        descriptorPoolInfo.pPoolSizes = poolSizes.data();                               // Pool Sizes to create with
        descriptorPoolInfo.flags = poolFlags;

        // Create Descriptor Pool
        VkResult result = vkCreateDescriptorPool(RVKDevice::s_rvkDevice->GetLogicalDevice(), &descriptorPoolInfo, nullptr, &m_descriptorPool);
        VK_CHECK(result, "Failed to create a Descriptor Pool!");
    }

    RVKDescriptorPool::~RVKDescriptorPool() {
        vkDestroyDescriptorPool(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_descriptorPool, nullptr);
    }

    bool RVKDescriptorPool::AllocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
        // Descriptor Set Allocation Info
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;                         // Pool to allocate Descriptor Set from
        allocInfo.pSetLayouts = &descriptorSetLayout;                        // Layouts to use allocate sets(1:1 relationship)
        allocInfo.descriptorSetCount = 1;                                    // Number of sets to allocate

        // Allocate descriptor sets(multiple)
        VkResult result = vkAllocateDescriptorSets(RVKDevice::s_rvkDevice->GetLogicalDevice(), &allocInfo, &descriptor);
        VK_CHECK(result, "Failed to allocate Descriptor Set!");
        if (result != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    void RVKDescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_descriptorPool, static_cast<u32>(descriptors.size()), descriptors.data());
    }

    void RVKDescriptorPool::ResetPool() {
        vkResetDescriptorPool(RVKDevice::s_rvkDevice->GetLogicalDevice(), m_descriptorPool, 0);
    }

    // *************** Descriptor Writer *********************
    RVKDescriptorWriter::RVKDescriptorWriter(RVKDescriptorSetLayout& setLayout, RVKDescriptorPool& pool)
        : m_setLayout{ setLayout }, m_pool{ pool } {}

    RVKDescriptorWriter& RVKDescriptorWriter::WriteBuffer(
        u32 binding, const VkDescriptorBufferInfo& bufferInfo) {
        VK_CORE_ASSERT(m_setLayout.m_bindings.count(binding) == 1, "Layout does not contain specified binding");

        auto& bindingDescription = m_setLayout.m_bindings[binding];

        VK_CORE_ASSERT(bindingDescription.descriptorCount == 1, "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = &bufferInfo;
        write.descriptorCount = 1;

        m_writes.push_back(write);
        return *this;
    }

    RVKDescriptorWriter& RVKDescriptorWriter::WriteImage(
        u32 binding, const VkDescriptorImageInfo& imageInfo) {
        VK_CORE_ASSERT(m_setLayout.m_bindings.count(binding) == 1, "Layout does not contain specified binding");

        auto& bindingDescription = m_setLayout.m_bindings[binding];

        VK_CORE_ASSERT(bindingDescription.descriptorCount == 1, "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = &imageInfo;
        write.descriptorCount = 1;

        m_writes.push_back(write);
        return *this;
    }

    bool RVKDescriptorWriter::Build(VkDescriptorSet& set) {
        bool success = m_pool.AllocateDescriptor(m_setLayout.GetDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        Overwrite(set);
        return true;
    }

    void RVKDescriptorWriter::Overwrite(VkDescriptorSet& set) {
        for (auto& write : m_writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(RVKDevice::s_rvkDevice->GetLogicalDevice(), static_cast<u32>(m_writes.size()), m_writes.data(), 0, nullptr);
    }
}
