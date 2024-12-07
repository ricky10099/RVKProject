#pragma once

#include "lve_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace RVK {

class LveDescriptorSetLayout {
 public:
  class Builder {
   public:
    Builder(RVKDevice &lveDevice) : lveDevice{lveDevice} {}

    Builder &addBinding(
        u32 binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        u32 count = 1);
    std::unique_ptr<LveDescriptorSetLayout> build() const;

   private:
    RVKDevice &lveDevice;
    std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings{};
  };

  LveDescriptorSetLayout(
      RVKDevice &lveDevice, std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings);
  ~LveDescriptorSetLayout();
  LveDescriptorSetLayout(const LveDescriptorSetLayout &) = delete;
  LveDescriptorSetLayout &operator=(const LveDescriptorSetLayout &) = delete;

  VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

 private:
  RVKDevice &lveDevice;
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings;

  friend class LveDescriptorWriter;
};

class RVKDescriptorPool {
 public:
  class Builder {
   public:
    Builder(RVKDevice &lveDevice) : lveDevice{lveDevice} {}

    Builder &addPoolSize(VkDescriptorType descriptorType, u32 count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(u32 count);
    std::unique_ptr<RVKDescriptorPool> build() const;

   private:
    RVKDevice &lveDevice;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    u32 maxSets = 1000;
    VkDescriptorPoolCreateFlags poolFlags = 0;
  };

  RVKDescriptorPool(
      RVKDevice &lveDevice,
      u32 maxSets,
      VkDescriptorPoolCreateFlags poolFlags,
      const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~RVKDescriptorPool();
  RVKDescriptorPool(const RVKDescriptorPool &) = delete;
  RVKDescriptorPool &operator=(const RVKDescriptorPool &) = delete;

  bool allocateDescriptor(
      const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

  void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

  void resetPool();

 private:
  RVKDevice &lveDevice;
  VkDescriptorPool descriptorPool;

  friend class LveDescriptorWriter;
};

class LveDescriptorWriter {
 public:
  LveDescriptorWriter(LveDescriptorSetLayout &setLayout, RVKDescriptorPool &pool);

  LveDescriptorWriter &writeBuffer(u32 binding, VkDescriptorBufferInfo *bufferInfo);
  LveDescriptorWriter &writeImage(u32 binding, VkDescriptorImageInfo *imageInfo);

  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);

 private:
  LveDescriptorSetLayout &setLayout;
  RVKDescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};

}  // namespace RVK
