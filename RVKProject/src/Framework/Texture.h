#pragma once
#include "Framework/Vulkan/VkUtils.h"

namespace RVK {
	class Texture {
	public:
		static constexpr bool USE_SRGB = true;
		static constexpr bool USE_UNORM = false;

	public:
		Texture(bool nearestFilter = false);
		Texture(u32 ID, int internalFormat, int dataFormat, int type);

		~Texture();

		bool Init(const u32 width, const u32 height, bool sRGB, const void* data, int minFilter, int magFilter);
		bool Init(const std::string& fileName, bool sRGB, bool flip = true);
		bool Init(const unsigned char* data, int length, bool sRGB);

		void Resize(u32 width, u32 height);
		void Blit(u32 x, u32 y, u32 width, u32 height, u32 bytesPerPixel, const void* data);
		void Blit(u32 x, u32 y, u32 width, u32 height, int dataFormat, int type, const void* data);
		void SetFilename(const std::string& filename) { m_fileName = filename; }

		VkDescriptorImageInfo& GetDescriptorImageInfo() { return m_descriptorImageInfo; }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }


	private:
        bool Create();
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);
        void CreateImage(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
        void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void GenerateMipmaps();

        VkFilter SetFilter(int minMagFilter);
        VkFilter SetFilterMip(int minFilter);

    private:
        std::string m_fileName;
        u32 m_rendererID;
        u8* m_localBuffer;
        int m_width, m_height, m_bytesPerPixel;
        u32 m_mipLevels;

        int m_internalFormat, m_dataFormat;
        bool m_sRGB;
        VkFilter m_minFilter;
        VkFilter m_magFilter;
        VkFilter m_minFilterMip;
        int m_type;

        VkFormat m_imageFormat;
        VkImage m_textureImage;
        VkDeviceMemory m_textureImageMemory;
        VkImageLayout m_imageLayout;
        VkImageView m_imageView;
        VkSampler m_sampler;

        VkDescriptorImageInfo m_descriptorImageInfo;

    private:
        static constexpr int TEXTURE_FILTER_NEAREST = 9728;
        static constexpr int TEXTURE_FILTER_LINEAR = 9729;
        static constexpr int TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST = 9984;
        static constexpr int TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST = 9985;
        static constexpr int TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR = 9986;
        static constexpr int TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR = 9987;
	};
}