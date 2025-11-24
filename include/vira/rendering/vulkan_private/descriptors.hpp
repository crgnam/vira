#ifndef VIRA_RENDERING_VULKAN_DESCRIPTORS_HPP
#define VIRA_RENDERING_VULKAN_DESCRIPTORS_HPP

#include <memory>
#include <unordered_map>
#include <vector>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/device.hpp"

namespace vira::vulkan {

    /**
     * @class ViraDescriptorSetLayout
     * @brief Manages a descriptor set layout defining descriptor set bindings.
	 * 
     * Descriptor Set Layouts define the format/layout of the descriptor bindings.
     * They describe the inidividual bindings, their types, and their shader stage access.
     * This class includes a builder class to help construct a new layout.
     * 
     */	
    class ViraDescriptorSetLayout {
    public:

        /**
         * @class ViraDescriptorSetLayout::Builder
         * @brief Builder class for constructing a ViraDescriptorSetLayout.
         *
         * This class follows the builder pattern to simplify the creation of a ViraDescriptorSetLayout.
         * It allows you to add bindings individually before finalizing the build.
         *
         * Example usage:
         * @code
         * std::unique_ptr<ViraDescriptorSetLayout> cameraDescriptorSetLayout = ViraDescriptorSetLayout::Builder(viraDevice)
         *      .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
         *      .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
         *      .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)               
         *      .build();
         * @endcode
         *
         * @see ViraDescriptorSetLayout
         */
        class Builder {
        public:
            Builder(ViraDevice* viraDevice) : viraDevice{ viraDevice } {}

            /**
             * Add a binding to the builder to be written by the build command.
             */            
            Builder& addBinding(
                uint32_t binding,
                vk::DescriptorType descriptorType,
                vk::ShaderStageFlags stageFlags,
                uint32_t count = 1);

            /**
             * Build the descriptor set layout.
             */            
            std::unique_ptr<ViraDescriptorSetLayout> build() const;

            std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> getBindings();
        
        private:
            ViraDevice* viraDevice;
            std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings{};
        };

        // Constructor / Destructor
        ViraDescriptorSetLayout(
            ViraDevice* viraDevice, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings);
        ~ViraDescriptorSetLayout();

        // Delete Copy Methods
        ViraDescriptorSetLayout(const ViraDescriptorSetLayout&) = delete;
        ViraDescriptorSetLayout& operator=(const ViraDescriptorSetLayout&) = delete;

        // Get Methods
        vk::DescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
        vk::DescriptorSetLayout* getpDescriptorSetLayout() { return &descriptorSetLayout; }

    protected:
        ViraDevice* viraDevice;
        vk::DescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings;

        friend class ViraDescriptorWriter;
    };

    /**
     * @class ViraDescriptorPool
     * @brief Manages a descriptor pool used for descriptor allocation.
	 * 
     * Descriptor pools define and allocate the descriptors for use by the pipeline.
     * 
     * This class includes a builder class to help construct a new descriptor pool.
     * 
     * Member Variables:
     * - ViraDevice* viraDevice: Pointer to the Vulkan device used to create the descriptor pool.
     * - vk::DescriptorPool descriptorPool: vulkan descriptor pool 
     * 
     */	
    class ViraDescriptorPool {
    public:
        /**
         * @class ViraDescriptorPool::Builder
         * @brief Builder class for constructing a ViraDescriptorPool.
         *
         * This class follows the builder pattern to simplify the creation of a ViraDescriptorPool.
         * It allows you to add pool sizes for different descriptor types individually before finalizing the build.
         *
         * Member Variables:
         * - ViraDevice* viraDevice: Pointer to the Vulkan device used to create the descriptor pool.
         * - std::vector<vk::DescriptorPoolSize> poolSizes: List of descriptor types and their counts to be allocated.
         * - uint32_t maxSets: Maximum number of descriptor sets that can be allocated from the pool (default: 1000).
         * - vk::DescriptorPoolCreateFlags poolFlags: Optional creation flags for the descriptor pool.
         *
         * Example usage:
         * @code
         * cameraDescriptorPool = ViraDescriptorPool::Builder(viraDevice)
         *      .addPoolSize(vk::DescriptorType::eUniformBuffer, 1) 
         *      .addPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
         *      .addPoolSize(vk::DescriptorType::eStorageBuffer, 1)
         *      .setMaxSets(1)
         *      .build();
         * @endcode
         * 
         * @see ViraDescriptorPool
         */
        class Builder {
        public:
            Builder(ViraDevice* viraDevice) : viraDevice{ viraDevice } {}

            /**
             * Add a descriptor type pool sized.
             */            
            Builder& addPoolSize(vk::DescriptorType descriptorType, uint32_t count);

            /**
             * Set Pool create flags.
             */            
            Builder& setPoolFlags(vk::DescriptorPoolCreateFlags flags);

            /**
             * Specify the maximum number of sets of descriptors specified by the pool sizes.
             */            
            Builder& setMaxSets(uint32_t count);

            /**
             * Build the descriptor pool
             */            
            std::unique_ptr<ViraDescriptorPool> build() const;

            // Get Methods
            std::vector<vk::DescriptorPoolSize> getPoolSizes() const;
            vk::DescriptorPoolCreateFlags getPoolFlags() const;
            uint32_t getMaxSets() const;

        private:
            
            ViraDevice* viraDevice;
            std::vector<vk::DescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            vk::DescriptorPoolCreateFlags poolFlags;
        };

        // Constructor / Destructor
        ViraDescriptorPool(
            ViraDevice* viraDevice,
            uint32_t maxSets,
            vk::DescriptorPoolCreateFlags poolFlags,
            const std::vector<vk::DescriptorPoolSize>& poolSizes);
        ~ViraDescriptorPool();

        // Delete Copy Methods
        ViraDescriptorPool(const ViraDescriptorPool&) = delete;
        ViraDescriptorPool& operator=(const ViraDescriptorPool&) = delete;

        /**
         * Allocate a descriptor set using provided descriptorSetLayout
         */            
        bool allocateDescriptorSet(
            const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet& descriptor) const;
        vk::DescriptorPool& getDescriptorPool() { return descriptorPool;};

        /**
         * Reset the descriptor pool
         */            
        void resetPool();

    private:
        ViraDevice* viraDevice;
        vk::DescriptorPool descriptorPool;

        friend class ViraDescriptorWriter;
    };

    /**
     * @class ViraDescriptorWriter
     * @brief Utility class for writing and updating Vulkan descriptor sets.
     *
     * ViraDescriptorWriter simplifies the process of writing various types of descriptors 
     * (buffers, images, acceleration structures, etc.) to Vulkan descriptor sets. It supports 
     * chaining write operations and can either build a new descriptor set or overwrite an existing one.
     *
     * The writer uses a specified descriptor set layout and a descriptor pool to allocate and manage descriptor sets.
     *
     * Example usage:
     * @code
     * ViraDescriptorWriter cameraDescriptorWriter = ViraDescriptorWriter(*cameraDescriptorSetLayout, *cameraDescriptorPool)
     *      .write(0, &cameraBufferInfo)
     *      .write(1, &pixelSolidAngleImageInfo)
     *      .write(2, &optEffBufferInfo)
     *      .build(cameraDescriptorSet);
     * @endcode
     *
     * Member Variables:
     * - ViraDescriptorSetLayout& setLayout: Reference to the layout that the descriptor set will conform to.
     * - ViraDescriptorPool& pool: Reference to the descriptor pool used for allocation.
     * - std::vector<vk::WriteDescriptorSet> writes: Internal list of pending descriptor writes.
     */
    class ViraDescriptorWriter {
    public:
        ViraDescriptorWriter(ViraDescriptorSetLayout& setLayout, ViraDescriptorPool& pool);

        ViraDescriptorWriter& writeBuffer(uint32_t binding, vk::DescriptorBufferInfo* bufferInfo);
        ViraDescriptorWriter& writeImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo);

        template <typename DescriptorInfoType>
        ViraDescriptorWriter& write( uint32_t binding, DescriptorInfoType* info);
        ViraDescriptorWriter& writeTextures(uint32_t binding, std::vector<vk::DescriptorImageInfo> imageInfos);

        ViraDescriptorWriter& writeAccelStructDescriptor(uint32_t binding, vk::WriteDescriptorSetAccelerationStructureKHR* info);

        ViraDescriptorWriter& writeImageDescriptor(uint32_t binding, vk::DescriptorImageInfo* info);

        ViraDescriptorWriter& writeBufferDescriptor(uint32_t binding, vk::DescriptorBufferInfo* info);


        ViraDescriptorWriter& writeBufferViews(uint32_t binding, const std::vector<vk::BufferView>& bufferViews);
        ViraDescriptorWriter& writeBufferArray(uint32_t binding, std::vector<vk::DescriptorBufferInfo> bufferInfos);

        bool build(vk::DescriptorSet& set);
        void overwrite(vk::DescriptorSet& set);

        // Delete the assignment operator and copy constructor
        ViraDescriptorWriter& operator=(const ViraDescriptorWriter&) = delete;
        ViraDescriptorWriter(const ViraDescriptorWriter&) = delete;
        
    private:
        ViraDescriptorSetLayout& setLayout;
        ViraDescriptorPool& pool;
        std::vector<vk::WriteDescriptorSet> writes;
    };

}

#include "implementation/rendering/vulkan_private/descriptors.ipp"

#endif