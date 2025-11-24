#ifndef VIRA_RENDERING_VULKAN_BUFFER_HPP
#define VIRA_RENDERING_VULKAN_BUFFER_HPP

#include "vulkan/vulkan.hpp"

// Vira private headers:
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"

namespace vira::vulkan {

    /**
     * @class ViraBuffer
     * @brief Encapsulates the creation, management, and access to Vulkan Buffers. 
     * @details This class encapsulates a Vulkan buffer, giving easy methods to create, access, write, and manipulate vulkan buffers.
     * @details
     * Member variables:
     * - ViraDevice* viraDevice: Pointer to the ViraDevice used to create the buffer.
     * - void* mapped: Pointer to host-visible mapped memory, or nullptr if unmapped.
     * - vk::UniqueBuffer buffer: Vulkan buffer handle with RAII ownership.
     * - std::unique_ptr<MemoryInterface> memory: Allocated device memory backing the buffer.
     * - vk::DeviceSize bufferSize: Total size of the buffer in bytes.
     * - uint32_t instanceCount: Number of instances stored in the buffer.
     * - vk::DeviceSize instanceSize: Size of a single instance in bytes.
     * - vk::DeviceSize alignmentSize: Aligned size per instance based on device requirements.
     * - vk::BufferUsageFlags usageFlags: Vulkan buffer usage flags (e.g., vertex, storage).
     * - vk::MemoryPropertyFlags memoryPropertyFlags: Memory property flags (e.g., device local, host visible).
	 * 	
     */
    class ViraBuffer {
    public:

        // Constructors/Destructors
        ViraBuffer();
        ViraBuffer(ViraDevice* device);
        ViraBuffer(
            ViraDevice* device,
            vk::DeviceSize instanceSize,
            uint32_t instanceCount,
            vk::BufferUsageFlags usageFlags,
            vk::MemoryPropertyFlags memoryPropertyFlags,
            vk::DeviceSize minOffsetAlignment = 1
        );
        ~ViraBuffer();

        // Delete Copy Methods
        ViraBuffer(const ViraBuffer&) = delete;
        ViraBuffer& operator=(const ViraBuffer&) = delete;

        // Get/Set/Move Property Methods
        vk::Buffer getBuffer() const { return buffer.get(); };
        vk::UniqueBuffer moveUniqueBuffer() {return std::move(buffer);};
        vk::BufferUsageFlags getUsageFlags() const { return usageFlags; };

        MemoryInterface& getMemory() {return *memory;};
        void* getMappedMemory() const { return mapped; };
        vk::MemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; };

        uint32_t getInstanceCount() const { return instanceCount; };
        vk::DeviceSize getInstanceSize() const { return instanceSize; };
        vk::DeviceSize getAlignmentSize() const { return alignmentSize; };
        vk::DeviceSize getBufferSize() const { return bufferSize; };

        /**
         * Returns the minimum instance size required to be compatible with vulkan device's minOffsetAlignment
         *
         * @param instanceSize The size of an instance
         * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
         * minUniformBufferOffsetAlignment)
         *
         * @return vk::Result of the buffer mapping call
         */
        static vk::DeviceSize getAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment);

        /**
         * Returns the vulkan buffer's device address for reference in the pipeline.
         *
         * @return vk::DeviceAddress The device address of the vulkan buffer.
         */
        vk::DeviceAddress getBufferDeviceAddress();

        /**
         * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
         *
         * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
         * buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return vk::Result of the buffer mapping call
         */
        vk::Result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

        /**
         * Unmap a mapped memory range
         *
         * @note Does not return a result as vkUnmapMemory can't fail
         */
        void unmap();

        /**
         * Flush a memory range of the buffer to make it visible to the device
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
         * complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return vk::Result of the flush call
         */
        virtual vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

        /**
         *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
         *
         * @param index Used in offset calculation
         *
         */
        vk::Result flushIndex(int index);

        /**
         * Copies the specified data to the mapped buffer. Default value writes whole buffer range
         *
         * @param data Pointer to the data to copy
         * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
         * range.
         * @param offset (Optional) Byte offset from beginning of mapped region
         *
         */        
        virtual void writeToBuffer(const void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
   
        /**
         * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
         *
         * @param data Pointer to the data to copy
         * @param index Used in offset calculation
         *
         */        
        void writeToIndex(void* data, int index);

        /**
         * Invalidate a memory range of the buffer to make it visible to the host
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
         * the complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return vk::Result of the invalidate call
         */
        virtual vk::Result invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

        /**
         * Invalidate a memory range of the buffer to make it visible to the host
         *
         * @note Only required for non-coherent memory
         *
         * @param index Specifies the region to invalidate: index * alignmentSize
         *
         * @return vk::Result of the invalidate call
         */
        vk::Result invalidateIndex(int index);


        /**
         * Create a buffer info descriptor
         *
         * @param size (Optional) Size of the memory range of the descriptor
         * @param offset (Optional) Byte offset from beginning
         *
         * @return vk::DescriptorBufferInfo of specified offset and range
         */
        virtual vk::DescriptorBufferInfo descriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

        /**
         * Create a buffer info descriptor
         *
         * @param index Specifies the region given by index * alignmentSize
         *
         * @return vk::DescriptorBufferInfo for instance at index
         */
        virtual vk::DescriptorBufferInfo descriptorInfoForIndex(int index);


    protected:

        ViraDevice* viraDevice;
        vk::DeviceSize instanceSize;
        uint32_t instanceCount;
        vk::BufferUsageFlags usageFlags;
        vk::MemoryPropertyFlags memoryPropertyFlags;
        void* mapped = nullptr;
        vk::UniqueBuffer buffer;
        std::unique_ptr<MemoryInterface> memory = VK_NULL_HANDLE;
        vk::DeviceSize bufferSize;
        vk::DeviceSize alignmentSize;
        
    };

}
#ifdef VIRA_DEVELOP
#include "implementation/rendering/vulkan_private/buffer.ipp"
#endif 

#endif