#include <cassert>
#include <stdexcept>
#include <iostream>

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

    // *************** Descriptor Set Layout Builder *********************

    ViraDescriptorSetLayout::Builder& ViraDescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        vk::DescriptorType descriptorType,
        vk::ShaderStageFlags stageFlags,
        uint32_t count)
    {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        vk::DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.setBinding(binding);
        layoutBinding.setDescriptorType(descriptorType);
        layoutBinding.setDescriptorCount(count);
        layoutBinding.setStageFlags(stageFlags);
        bindings[binding] = layoutBinding;
        return *this;
    };

    std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> ViraDescriptorSetLayout::Builder::getBindings() 
    {
        return bindings;
    };

    std::unique_ptr<ViraDescriptorSetLayout> ViraDescriptorSetLayout::Builder::build() const
    {
        return std::make_unique<ViraDescriptorSetLayout>(viraDevice, bindings);
    };

    // *************** Descriptor Set Layout *********************

    ViraDescriptorSetLayout::ViraDescriptorSetLayout(
        ViraDevice* viraDevice, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings)
        : viraDevice{ viraDevice }, bindings{ bindings }
    {
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        try {
            descriptorSetLayout = viraDevice->device()->createDescriptorSetLayout(descriptorSetLayoutInfo, nullptr);
        } catch (const vk::SystemError& err) {
            std::cerr << "Failed to create descriptor set layout: " << err.what() << std::endl;
        }
        
    };

    ViraDescriptorSetLayout::~ViraDescriptorSetLayout()
    {
        viraDevice->device()->destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
    };

    // *************** Descriptor Pool Builder *********************

    ViraDescriptorPool::Builder& ViraDescriptorPool::Builder::addPoolSize(
        vk::DescriptorType descriptorType, uint32_t count)
    {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    };

    ViraDescriptorPool::Builder& ViraDescriptorPool::Builder::setPoolFlags(
        vk::DescriptorPoolCreateFlags flags)
    {
        poolFlags = flags;
        return *this;
    };

    ViraDescriptorPool::Builder& ViraDescriptorPool::Builder::setMaxSets(uint32_t count)
    {
        maxSets = count;
        return *this;
    };

    std::unique_ptr<ViraDescriptorPool> ViraDescriptorPool::Builder::build() const
    {
        return std::make_unique<ViraDescriptorPool>(viraDevice, maxSets, poolFlags, poolSizes);
    };

    std::vector<vk::DescriptorPoolSize> ViraDescriptorPool::Builder::getPoolSizes() const 
    {
        return poolSizes;
    };

    vk::DescriptorPoolCreateFlags ViraDescriptorPool::Builder::getPoolFlags() const 
    {
        return poolFlags;
    };

    uint32_t  ViraDescriptorPool::Builder::getMaxSets() const 
    {
        return maxSets;
    }

    // *************** Descriptor Pool *********************

    ViraDescriptorPool::ViraDescriptorPool(
        ViraDevice* viraDevice,
        uint32_t maxSets,
        vk::DescriptorPoolCreateFlags poolFlags,
        const std::vector<vk::DescriptorPoolSize>& poolSizes)
        : viraDevice{ viraDevice }
    {
        vk::DescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()));
        descriptorPoolInfo.setPoolSizes(poolSizes);
        descriptorPoolInfo.setMaxSets(maxSets);
        descriptorPoolInfo.setFlags(poolFlags);

        try {
            descriptorPool = viraDevice->device()->createDescriptorPool(descriptorPoolInfo, nullptr);
        } catch (vk::SystemError& err) {
            std::cerr << "Failed to create descriptor pool: " << err.what() << std::endl;
        }
    };

    ViraDescriptorPool::~ViraDescriptorPool()
    {
        viraDevice->device()->destroyDescriptorPool(descriptorPool, nullptr);
    };

    bool ViraDescriptorPool::allocateDescriptorSet(
        const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet& descriptor) const
    {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(descriptorPool);
        allocInfo.setPSetLayouts(&descriptorSetLayout);
        allocInfo.setDescriptorSetCount(1);
        

        try {
            std::vector<vk::DescriptorSet> descriptors = viraDevice->device()->allocateDescriptorSets(allocInfo);
            descriptor = descriptors[0];
            return true;
        } catch (vk::SystemError& err) {
            std::cerr << "Failed to allocate descriptor sets: " << err.what() << std::endl;
            return false;
        }
    };

    void ViraDescriptorPool::resetPool()
    {
        viraDevice->device()->resetDescriptorPool(descriptorPool, vk::DescriptorPoolResetFlags{});
    };

    // *************** Descriptor Writer *********************

    ViraDescriptorWriter::ViraDescriptorWriter(ViraDescriptorSetLayout& setLayout, ViraDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {}

    template <typename DescriptorInfoType>
    ViraDescriptorWriter& ViraDescriptorWriter::write(uint32_t binding, DescriptorInfoType* info)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        assert(info != nullptr && "Descriptor info cannot be null");
        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(bindingDescription.descriptorCount);

        if constexpr (std::is_same_v<DescriptorInfoType, vk::DescriptorBufferInfo>) {
            assert(descriptorType == vk::DescriptorType::eUniformBuffer || 
                descriptorType == vk::DescriptorType::eStorageBuffer);

            if (bindingDescription.descriptorCount == 1) {
                // Single descriptor case
                write.setBufferInfo(*static_cast<vk::DescriptorBufferInfo*>(info));
            } else {
                // Multiple descriptors case
                auto descriptorInfos = static_cast<const vk::DescriptorBufferInfo*>(info);
                write.setBufferInfo(vk::ArrayProxyNoTemporaries<const vk::DescriptorBufferInfo>(bindingDescription.descriptorCount, descriptorInfos));
            }

        } else if constexpr (std::is_same_v<DescriptorInfoType, vk::DescriptorImageInfo>) {
            assert(descriptorType == vk::DescriptorType::eCombinedImageSampler || 
                descriptorType == vk::DescriptorType::eSampledImage || 
                descriptorType == vk::DescriptorType::eStorageImage);

            if (bindingDescription.descriptorCount == 1) {
                write.setImageInfo(*static_cast<vk::DescriptorImageInfo*>(info));
            } else {
                auto descriptorInfos = static_cast<const vk::DescriptorImageInfo*>(info);
                write.setImageInfo(vk::ArrayProxyNoTemporaries<const vk::DescriptorImageInfo>(bindingDescription.descriptorCount, descriptorInfos));            
            }

        } else if constexpr (std::is_same_v<DescriptorInfoType, vk::WriteDescriptorSetAccelerationStructureKHR>) {
            assert(descriptorType == vk::DescriptorType::eAccelerationStructureKHR);
            write.setPNext(info);

        } else {
            throw std::runtime_error("Unsupported descriptor type for write operation");
        }

        writes.push_back(write);
        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeBufferDescriptor(uint32_t binding, vk::DescriptorBufferInfo* info)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        assert(info != nullptr && "Descriptor info cannot be null");
        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(bindingDescription.descriptorCount);

        assert(descriptorType == vk::DescriptorType::eUniformBuffer || 
            descriptorType == vk::DescriptorType::eStorageBuffer);

        if (bindingDescription.descriptorCount == 1) {
            // Single descriptor case
            write.setBufferInfo(*static_cast<vk::DescriptorBufferInfo*>(info));
        } else {
            // Multiple descriptors case
            auto descriptorInfos = static_cast<const vk::DescriptorBufferInfo*>(info);
            write.setBufferInfo(vk::ArrayProxyNoTemporaries<const vk::DescriptorBufferInfo>(bindingDescription.descriptorCount, descriptorInfos));
        }

        writes.push_back(write);
        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeImageDescriptor(uint32_t binding, vk::DescriptorImageInfo* info)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        assert(info != nullptr && "Descriptor info cannot be null");
        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(bindingDescription.descriptorCount);

        assert(descriptorType == vk::DescriptorType::eCombinedImageSampler || 
            descriptorType == vk::DescriptorType::eSampledImage || 
            descriptorType == vk::DescriptorType::eStorageImage);

        if (bindingDescription.descriptorCount == 1) {
            write.setImageInfo(*static_cast<vk::DescriptorImageInfo*>(info));
        } else {
            auto descriptorInfos = static_cast<const vk::DescriptorImageInfo*>(info);
            write.setImageInfo(vk::ArrayProxyNoTemporaries<const vk::DescriptorImageInfo>(bindingDescription.descriptorCount, descriptorInfos));            
        }

        writes.push_back(write);
        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeAccelStructDescriptor(uint32_t binding, vk::WriteDescriptorSetAccelerationStructureKHR* info)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
        assert(info != nullptr && "Descriptor info cannot be null");

        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        // Ensure the descriptor type matches
        assert(descriptorType == vk::DescriptorType::eAccelerationStructureKHR);

        // Ensure descriptor count matches
        assert(info->accelerationStructureCount == bindingDescription.descriptorCount && "Mismatch in descriptor count");

        // Create the descriptor write
        vk::WriteDescriptorSet write{};
        write.setDescriptorType(descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(bindingDescription.descriptorCount)
            .setPNext(info);

        // Add to the list of writes
        writes.push_back(write);

        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeBuffer(
        uint32_t binding, vk::DescriptorBufferInfo* bufferInfo)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(bindingDescription.descriptorType)
             .setDstBinding(binding)
             .setBufferInfo(*bufferInfo)
             .setDescriptorCount(1);

        writes.push_back(write);
        return *this;
    };

    ViraDescriptorWriter& ViraDescriptorWriter::writeImage(
        uint32_t binding, vk::DescriptorImageInfo* imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(bindingDescription.descriptorType)
             .setDstBinding(binding)
             .setImageInfo(*imageInfo)
             .setDescriptorCount(1);

        writes.push_back(write);
        return *this;
    };

    bool ViraDescriptorWriter::build(vk::DescriptorSet& set)
    {
        bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        overwrite({set});
        return true;
    };

    void ViraDescriptorWriter::overwrite(vk::DescriptorSet& set)
    {
        for (auto& write : writes) {
            write.dstSet = set;
        }
        pool.viraDevice->device()->updateDescriptorSets(writes, {});
    };

    ViraDescriptorWriter& ViraDescriptorWriter::writeTextures(uint32_t binding, std::vector<vk::DescriptorImageInfo> imageInfos) {

        uint32_t descriptorCount = static_cast<uint32_t>(imageInfos.size());

        // Ensure the binding exists in the layout
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        // Retrieve binding description
        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        // Ensure the provided descriptor count matches the binding layout
        assert(
            bindingDescription.descriptorCount >= imageInfos.size() &&
            "Provided descriptor count exceeds the layout's descriptor count");

        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(bindingDescription.descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(descriptorCount);

        // Ensure the descriptor type is compatible with image textures
        if (descriptorType == vk::DescriptorType::eCombinedImageSampler ||
            descriptorType == vk::DescriptorType::eSampledImage ||
            descriptorType == vk::DescriptorType::eStorageImage) {
            write.setImageInfo(imageInfos);
        } else {
            throw std::runtime_error("Unsupported descriptor type for writing image textures");
        }

        // Add the write operation to the list
        writes.push_back(write);
        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeBufferArray(uint32_t binding, std::vector<vk::DescriptorBufferInfo> bufferInfos) {

        uint32_t descriptorCount = static_cast<uint32_t>(bufferInfos.size());

        // Ensure the binding exists in the layout
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        // Retrieve binding description
        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        // Ensure the provided descriptor count matches the binding layout
        assert(
            bindingDescription.descriptorCount >= bufferInfos.size() &&
            "Provided descriptor count exceeds the layout's descriptor count");

        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(bindingDescription.descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(descriptorCount);

        // Ensure the descriptor type is compatible with image textures
        if (descriptorType == vk::DescriptorType::eUniformBuffer ||
            descriptorType == vk::DescriptorType::eStorageBuffer) {
            write.setBufferInfo(bufferInfos);
        } else {
            throw std::runtime_error("Unsupported descriptor type for writing buffer arrays");
        }

        // Add the write operation to the list
        writes.push_back(write);
        return *this;
    }

    ViraDescriptorWriter& ViraDescriptorWriter::writeBufferViews(uint32_t binding, const std::vector<vk::BufferView>& bufferViews) {
        // Ensure the binding exists in the layout
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        // Retrieve binding description
        auto& bindingDescription = setLayout.bindings[binding];
        vk::DescriptorType descriptorType = bindingDescription.descriptorType;

        // Ensure the descriptor type is compatible with buffer views
        assert(
            (descriptorType == vk::DescriptorType::eUniformTexelBuffer ||
            descriptorType == vk::DescriptorType::eStorageTexelBuffer) &&
            "Descriptor type must be eUniformTexelBuffer or eStorageTexelBuffer");

        // Ensure the provided descriptor count matches the binding layout
        assert(
            bindingDescription.descriptorCount >= bufferViews.size() &&
            "Provided descriptor count exceeds the layout's descriptor count");

        // Create the write descriptor set
        vk::WriteDescriptorSet write = {};
        write.setDescriptorType(descriptorType)
            .setDstBinding(binding)
            .setDescriptorCount(static_cast<uint32_t>(bufferViews.size()))
            .setTexelBufferView(bufferViews);

        // Add the write operation to the list
        writes.push_back(write);
        return *this;
    }

};