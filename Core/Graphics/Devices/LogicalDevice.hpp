#pragma once

#include "volk.h"
#include <mutex>
#include <optional>
#include <vector>

namespace MapleLeaf {
class Instance;
class PhysicalDevice;

class QueueWarpper
{
public:
    QueueWarpper() {}

    const VkQueue& GetGraphicsQueue() const { return graphicsQueue; }
    const VkQueue& GetComputeQueue() const { return computeQueue; }
    const VkQueue& GetTransferQueue() const { return transferQueue; }

    std::mutex& GetGraphicsQueueMutex() const { return graphicsQueueMutex; }
    std::mutex& GetComputeQueueMutex() const { return computeQueueMutex; }
    std::mutex& GetTransferQueueMutex() const { return transferQueueMutex; }

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue  = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;

    mutable std::mutex graphicsQueueMutex;
    mutable std::mutex computeQueueMutex;
    mutable std::mutex transferQueueMutex;
};

class LogicalDevice
{
    friend class Graphics;
    friend class QueueWarpper;

public:
    LogicalDevice(const Instance& instance, const PhysicalDevice& physicalDevice);
    ~LogicalDevice();

    operator const VkDevice&() const { return logicalDevice; }

    const VkDevice&                 GetLogicalDevice() const { return logicalDevice; }
    const VkPhysicalDeviceFeatures& GetEnabledFeatures() const { return enabledFeatures; }

    uint32_t GetGraphicsFamily() const { return graphicsFamily.value(); }
    uint32_t GetPresentFamily() const { return presentFamily.value(); }
    uint32_t GetComputeFamily() const { return computeFamily.value(); }
    uint32_t GetTransferFamily() const { return transferFamily.value(); }

    const VkQueue& GetPresentQueue() const { return presentQueue; }
    const VkQueue& GetSubmitGraphicsQueue() const { return submitQueueWarpper.GetGraphicsQueue(); }
    const VkQueue& GetSubmitComputeQueue() const { return submitQueueWarpper.GetComputeQueue(); }
    const VkQueue& GetSubmitTransferQueue() const { return submitQueueWarpper.GetTransferQueue(); }
    const VkQueue& GetIdleGraphicsQueue() const { return idleQueueWarpper.GetGraphicsQueue(); }
    const VkQueue& GetIdleComputeQueue() const { return idleQueueWarpper.GetComputeQueue(); }
    const VkQueue& GetIdleTransferQueue() const { return idleQueueWarpper.GetTransferQueue(); }

    std::mutex& GetSubmitGraphicsQueueMutex() const { return submitQueueWarpper.GetGraphicsQueueMutex(); }
    std::mutex& GetSubmitComputeQueueMutex() const { return submitQueueWarpper.GetComputeQueueMutex(); }
    std::mutex& GetSubmitTransferQueueMutex() const { return submitQueueWarpper.GetTransferQueueMutex(); }
    std::mutex& GetIdleGraphicsQueueMutex() const { return idleQueueWarpper.GetGraphicsQueueMutex(); }
    std::mutex& GetIdleComputeQueueMutex() const { return idleQueueWarpper.GetComputeQueueMutex(); }
    std::mutex& GetIdleTransferQueueMutex() const { return idleQueueWarpper.GetTransferQueueMutex(); }

    uint32_t GetBindlessMaxDescriptorsCount() const { return bindlessMaxDescriptorsCount; }

    const std::thread::id& GetMainThreadId() const { return mainThreadId; }
    bool                   IsMainThread() const { return std::this_thread::get_id() == mainThreadId; }

    static const std::vector<const char*> DeviceExtensions;

private:
    const Instance&       instance;
    const PhysicalDevice& physicalDevice;

    VkDevice                 logicalDevice   = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures enabledFeatures = {};

    VkQueueFlags            supportedQueues = {};
    std::optional<uint32_t> graphicsFamily  = std::nullopt;
    std::optional<uint32_t> presentFamily   = std::nullopt;
    std::optional<uint32_t> computeFamily   = std::nullopt;
    std::optional<uint32_t> transferFamily  = std::nullopt;

    QueueWarpper submitQueueWarpper;
    QueueWarpper idleQueueWarpper;
    VkQueue      presentQueue = VK_NULL_HANDLE;

    std::thread::id mainThreadId;

    uint32_t bindlessMaxDescriptorsCount = 0;

    void CreateQueueIndices();
    void CreateLogicalDevice();
};
}   // namespace MapleLeaf