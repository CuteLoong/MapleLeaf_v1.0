#include "Buffer.hpp"
#include "Graphics.hpp"
#include <array>

namespace MapleLeaf {
Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* data, bool align)
{
    auto logicalDevice  = Graphics::Get()->GetLogicalDevice();
    auto physicalDevice = Graphics::Get()->GetPhysicalDevice();

    this->size = align ? (size + physicalDevice->GetProperties().limits.nonCoherentAtomSize - 1) &
                             ~(physicalDevice->GetProperties().limits.nonCoherentAtomSize - 1)
                       : size;

    auto graphicsFamily = logicalDevice->GetGraphicsFamily();
    auto presentFamily  = logicalDevice->GetPresentFamily();
    auto computeFamily  = logicalDevice->GetComputeFamily();

    std::array queueFamily = {graphicsFamily, presentFamily, computeFamily};

    // Create the buffer handle.
    VkBufferCreateInfo bufferCreateInfo    = {};
    bufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size                  = this->size;
    bufferCreateInfo.usage                 = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;   // device address for ray tracing
    bufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamily.size());
    bufferCreateInfo.pQueueFamilyIndices   = queueFamily.data();
    Graphics::CheckVk(vkCreateBuffer(*logicalDevice, &bufferCreateInfo, nullptr, &buffer));

    // Create the memory backing up the buffer handle.
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
    memoryAllocateFlagsInfo.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags                     = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(*logicalDevice, buffer, &memoryRequirements);
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize       = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex      = FindMemoryType(memoryRequirements.memoryTypeBits, properties);
    memoryAllocateInfo.pNext                = &memoryAllocateFlagsInfo;
    Graphics::CheckVk(vkAllocateMemory(*logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory));

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data.
    if (data) {
        void* mapped;
        MapMemory(&mapped);
        std::memcpy(mapped, data, size);

        // If host coherency hasn't been requested, do a manual flush to make writes visible.
        if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
            VkMappedMemoryRange mappedMemoryRange = {};
            mappedMemoryRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.memory              = bufferMemory;
            mappedMemoryRange.offset              = 0;
            mappedMemoryRange.size                = size;
            vkFlushMappedMemoryRanges(*logicalDevice, 1, &mappedMemoryRange);
        }

        UnmapMemory();
    }
    // Attach the memory to the buffer object.
    Graphics::CheckVk(vkBindBufferMemory(*logicalDevice, buffer, bufferMemory, 0));

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
    bufferDeviceAddressInfo.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer                    = buffer;
    deviceAddress                                     = vkGetBufferDeviceAddress(*logicalDevice, &bufferDeviceAddressInfo);
}

Buffer::~Buffer()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    if (logicalDevice->IsMainThread()) {
        std::lock_guard<std::mutex> lock(logicalDevice->GetSubmitGraphicsQueueMutex());
        Graphics::CheckVk(vkQueueWaitIdle(logicalDevice->GetSubmitGraphicsQueue()));
    }
    else {
        std::lock_guard<std::mutex> lock(logicalDevice->GetIdleGraphicsQueueMutex());
        Graphics::CheckVk(vkQueueWaitIdle(logicalDevice->GetIdleGraphicsQueue()));
    }
    // Graphics::CheckVk(vkDeviceWaitIdle(*logicalDevice));

    vkDestroyBuffer(*logicalDevice, buffer, nullptr);
    vkFreeMemory(*logicalDevice, bufferMemory, nullptr);
}

void Buffer::MapMemory(void** data) const
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();
    Graphics::CheckVk(vkMapMemory(*logicalDevice, bufferMemory, 0, size, 0, data));
}

void Buffer::UnmapMemory() const
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();
    vkUnmapMemory(*logicalDevice, bufferMemory);
}

void Buffer::FlushMappedMemory(VkDeviceSize size) const
{
    auto                logicalDevice     = Graphics::Get()->GetLogicalDevice();
    VkMappedMemoryRange mappedMemoryRange = {};
    mappedMemoryRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedMemoryRange.memory              = bufferMemory;
    mappedMemoryRange.offset              = 0;
    mappedMemoryRange.size                = size;
    vkFlushMappedMemoryRanges(*logicalDevice, 1, &mappedMemoryRange);
}

uint32_t Buffer::FindMemoryType(uint32_t typeFilter, const VkMemoryPropertyFlags& requiredProperties)
{
    auto physicalDevice   = Graphics::Get()->GetPhysicalDevice();
    auto memoryProperties = physicalDevice->GetMemoryProperties();

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        uint32_t memoryTypeBits = 1 << i;

        if (typeFilter & memoryTypeBits && (memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a valid memory type for buffer");
}

void Buffer::InsertBufferMemoryBarrier(const CommandBuffer& commandBuffer, const VkBuffer& buffer, VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                       VkDeviceSize offset, VkDeviceSize size)
{
    VkBufferMemoryBarrier bufferMemoryBarrier = {};
    bufferMemoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferMemoryBarrier.srcAccessMask         = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask         = dstAccessMask;
    bufferMemoryBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.buffer                = buffer;
    bufferMemoryBarrier.offset                = offset;
    bufferMemoryBarrier.size                  = size;
    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}
}   // namespace MapleLeaf