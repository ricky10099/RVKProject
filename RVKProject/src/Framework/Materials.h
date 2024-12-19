#pragma once

#include "Framework/Texture.h"
#include "Framework/Vulkan/SharedDefines.h"
#include "Framework/Vulkan/RVKBuffer.h"

namespace RVK {
    class MaterialDescriptor;
    class Material {
	public:        
        enum TextureIndices {
            DIFFUSE_MAP_INDEX = 0,
            NORMAL_MAP_INDEX,
            ROUGHNESS_MAP_INDEX,
            METALLIC_MAP_INDEX,
            ROUGHNESS_METALLIC_MAP_INDEX,
            EMISSIVE_MAP_INDEX,
            NUM_TEXTURES
        };

        enum MaterialFeatures // bitset
        {
            HAS_DIFFUSE_MAP = GLSL_HAS_DIFFUSE_MAP,
            HAS_NORMAL_MAP = GLSL_HAS_NORMAL_MAP,
            HAS_ROUGHNESS_MAP = GLSL_HAS_ROUGHNESS_MAP,
            HAS_METALLIC_MAP = GLSL_HAS_METALLIC_MAP,
            HAS_ROUGHNESS_METALLIC_MAP = GLSL_HAS_ROUGHNESS_METALLIC_MAP,
            HAS_EMISSIVE_COLOR = GLSL_HAS_EMISSIVE_COLOR,
            HAS_EMISSIVE_MAP = GLSL_HAS_EMISSIVE_MAP
        };

		struct PBRMaterial {
            // byte 0 to 15
            u32 features{ 0 };
            float roughness{ 0.0f };
            float metallic{ 0.0f };
            float spare0{ 0.0f }; // padding

            // byte 16 to 31
            glm::vec4 diffuseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

            // byte 32 to 47
            glm::vec3 emissiveColor{ 0.0f, 0.0f, 0.0f };
            float emissiveStrength{ 1.0f };

            // byte 48 to 63
            float normalMapIntensity{ 1.0f };
            float spare1{ 0.0f }; // padding
            float spare2{ 0.0f }; // padding
            float spare3{ 0.0f }; // padding

            // byte 64 to 128
            glm::vec4 spare4[4];
		};
        
        //struct PBRMaterial {
        //    u32 features{ 0 };
        //    float roughness{ 0.0f };
        //    float metallic{ 0.0f };

        //    // byte 16 to 31
        //    glm::vec4 diffuseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        //    // byte 32 to 47
        //    glm::vec3 emissiveColor{ 0.0f, 0.0f, 0.0f };
        //    float emissiveStrength{ 1.0f };

        //    // byte 48 to 63
        //    float normalMapIntensity{ 1.0f };
        //};

    public:
        using MaterialTextures = std::array<std::shared_ptr<Texture>, Material::NUM_TEXTURES>;

        std::shared_ptr<RVKBuffer> m_materialBuffer;
		PBRMaterial m_PBRMaterial;
        std::shared_ptr<MaterialDescriptor> m_materialDescriptor;
        MaterialTextures m_materialTextures;
	};
}