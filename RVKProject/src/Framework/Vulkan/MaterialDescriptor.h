#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Materials.h"

namespace RVK {
    class MaterialDescriptor {
    public:
        MaterialDescriptor(Material::MaterialTextures& textures);

        MaterialDescriptor(MaterialDescriptor const& other);

        virtual ~MaterialDescriptor() = default;

    public:
        const VkDescriptorSet& GetDescriptorSet() const { return m_descriptorSet; }

    private:
        VkDescriptorSet m_descriptorSet;
    };
} // namespace RVK
