#include <cassert>
#include <stdexcept>
#include <iostream>
#include <cstring>

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

    ViraBuffer::ViraBuffer() {};

    ViraBuffer::ViraBuffer(ViraDevice* viraDevice) : viraDevice{viraDevice} {};

    ViraBuffer::ViraBuffer(
        ViraDevice* viraDevice,
        vk::DeviceSize instanceSize,
        uint32_t instanceCount,
        vk::BufferUsageFlags usageFlags,
        vk::MemoryPropertyFlags memoryPropertyFlags,
        vk::DeviceSize minOffsetAlignment)
        : viraDevice{ viraDevice },
        instanceSize{ instanceSize },
        instanceCount{ instanceCount },
        usageFlags{ usageFlags },
        memoryPropertyFlags{ memoryPropertyFlags }
    {
        
        alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = instanceCount * alignmentSize;
        memory = viraDevice->createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer);

    };

    ViraBuffer::~ViraBuffer()
    {
        unmap();
        buffer.reset();      
        memory.reset();      
    };
    
    vk::DeviceSize ViraBuffer::getAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment)
    {
        if (minOffsetAlignment > 0) {
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    };

    vk::DeviceAddress ViraBuffer::getBufferDeviceAddress() {

        vk::BufferDeviceAddressInfo addressInfo{};
        addressInfo.buffer = buffer.get();

        vk::DeviceAddress deviceAddress = viraDevice->getBufferAddress(addressInfo);
        return deviceAddress;
        
    }

    vk::Result ViraBuffer::map(vk::DeviceSize size, vk::DeviceSize offset)
    {
        assert(buffer && memory && "Called map on buffer before create");
        
        try
        {
            mapped = viraDevice->device()->mapMemory(memory.get(), offset, size, vk::MemoryMapFlags{});
        }   
        catch (const vk::SystemError& err) {
            std::cerr << "Failed to map device memory: " << err.what() << std::endl;
        }
        return vk::Result::eSuccess;
    };

    void ViraBuffer::unmap()
    {
        if (mapped) {
            viraDevice->device()->unmapMemory(memory);
            mapped = nullptr;
        }
    };

    vk::Result ViraBuffer::flush(vk::DeviceSize size, vk::DeviceSize offset)
    {
        vk::MappedMemoryRange mappedRange = {};
        mappedRange.setMemory(memory->getMemory());
        mappedRange.setOffset(offset);
        mappedRange.setSize(size);
        try {
            viraDevice->device()->flushMappedMemoryRanges({ mappedRange });
        }
        catch (const std::exception& err) {
            std::cerr << "Vulkan error: " << err.what() << std::endl;
        }
        return vk::Result::eSuccess;
    };

    vk::Result ViraBuffer::flushIndex(int index)
    {
        vk::DeviceSize alignedSize = std::max(alignmentSize, viraDevice->getPhysicalDeviceProperties().limits.nonCoherentAtomSize);        
        return flush(alignedSize, index * alignmentSize);
    };
    
    vk::Result ViraBuffer::invalidate(vk::DeviceSize size, vk::DeviceSize offset)
    {

        vk::MappedMemoryRange mappedRange = {};
        mappedRange.setMemory(memory->getMemory());
        mappedRange.setOffset(offset);
        mappedRange.setSize(size);
        vk::Result result = viraDevice->device()->invalidateMappedMemoryRanges(1, &mappedRange);
        return result;

    };

    vk::Result ViraBuffer::invalidateIndex(int index)
    {
        vk::DeviceSize alignedSize = std::max(alignmentSize, viraDevice->getPhysicalDeviceProperties().limits.nonCoherentAtomSize);        

        return invalidate(alignedSize, index * alignmentSize);
    };
    
    void ViraBuffer::writeToBuffer(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        assert(mapped && "Cannot copy to unmapped buffer");

        if (size == vk::DeviceSize(VK_WHOLE_SIZE)) {
            memcpy(mapped, data, bufferSize);
        }
        else {
            char* memOffset = (char*)mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    };

    void ViraBuffer::writeToIndex(void* data, int index)
    {
        writeToBuffer(data, instanceSize, index * alignmentSize);
    };
    
    vk::DescriptorBufferInfo ViraBuffer::descriptorInfo(vk::DeviceSize size, vk::DeviceSize offset)
    {

        vk::DescriptorBufferInfo descriptorBufferInfo = {};

        descriptorBufferInfo.setBuffer(buffer.get());
        descriptorBufferInfo.setOffset(offset);
        descriptorBufferInfo.setRange(size);
        return descriptorBufferInfo;
    };
    
    vk::DescriptorBufferInfo ViraBuffer::descriptorInfoForIndex(int index) 
    {
        return descriptorInfo(alignmentSize, index * alignmentSize);
    };

}