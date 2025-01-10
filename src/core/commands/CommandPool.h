//
// Created by HUSTLX on 2025/1/7.
//

#ifndef HARDWAREPATHTRACER_COMMANDPOOL_H
#define HARDWAREPATHTRACER_COMMANDPOOL_H

#include "core/Core.h"
#include <optional>


namespace HWPT {

    enum PoolType {
        Graphics = 0x1,
        Compute = Graphics << 1,
        Transient = Compute << 1,
        Protected = Transient << 1,
        PoolTypeMax = 0x7fffffff
    };

    enum class QueueType {
        Graphics = 0x0,
        Compute,
        Present,
        QueueTypeMax = 0x7fffffff,
    };

    struct QueueFamilyIndices {
        std::optional<uint> GraphicsFamily;
        std::optional<uint> ComputeFamily;
        std::optional<uint> PresentFamily;

        [[nodiscard]] auto IsComplete() const -> bool {
            return GraphicsFamily.has_value() && ComputeFamily.has_value() &&
                   PresentFamily.has_value();
        }
    };

    class CommandPool {
    public:
        CommandPool(PoolType PoolType_) : m_poolType(PoolType_) {
            this->Init();
        }

        CommandPool(uint PoolType_) : m_poolType(static_cast<PoolType>(PoolType_)) {
            this->Init();
        }

        ~CommandPool();

        void Init();

        auto HasGraphicsCommandPool() const -> bool;

        auto HasComputeCommandPool() const -> bool;

        auto GetGraphicsPool() -> VkCommandPool {
            Check(HasGraphicsCommandPool());
            return m_graphicsPool;
        }

        auto GetComputePool() -> VkCommandPool {
            Check(HasComputeCommandPool());
            return m_computePool;
        }

        auto GetGraphicsQueue() -> VkQueue {
            Check(HasGraphicsCommandPool());
            return m_graphicsQueue;
        }

        auto GetComputeQueue() -> VkQueue {
            Check(HasComputeCommandPool());
            return m_computeQueue;
        }

        auto GetPresentQueue() -> VkQueue {
            Check(HasGraphicsCommandPool());
            return m_presentQueue;
        }

        auto GetQueueFamilyIndices() const -> const QueueFamilyIndices & {
            return m_queueFamilyIndices;
        }

        auto BeginCommandBuffer(QueueType QueueType_) -> VkCommandBuffer;

        void SubmitCommandBuffer(VkCommandBuffer CommandBuffer, QueueType QueueType_);

    protected:
        VkCommandPool m_graphicsPool = VK_NULL_HANDLE;
        VkCommandPool m_computePool = VK_NULL_HANDLE;
        PoolType m_poolType = PoolType::PoolTypeMax;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_computeQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;

        QueueFamilyIndices m_queueFamilyIndices;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_COMMANDPOOL_H
