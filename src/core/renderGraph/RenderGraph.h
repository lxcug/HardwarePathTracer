//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RENDERGRAPH_H
#define HARDWAREPATHTRACER_RENDERGRAPH_H

#include "RenderPassCommon.h"
#include <functional>


namespace HWPT {
    struct RenderPassMetaData {
        std::string PassName;
        RenderPassBase *Pass = nullptr;
        std::function<void()> ExecLambda;
    };

    class RenderGraph {
    public:
        RenderGraph();

        ~RenderGraph();

        void Create();

        void Destroy();

        void AddPass(const std::string &PassName, RenderPassBase *Pass,
                     const std::function<void()> &ExecLambda);

        auto Validate() -> bool;

        void Execute();

        void OnNewFrame();

        void SetFrameIndex(uint FrameIndex) {
            m_frameIndex = FrameIndex;
        }

        void SetImageAvailableSemaphore(VkSemaphore ImageAvailable) {
            m_imageAvailable = ImageAvailable;
        }

        void SetRenderFinishFence(VkFence RenderFinish) {
            m_renderFinish = RenderFinish;
        }

        auto GetRenderFinishSemaphore() -> VkSemaphore& {
            return m_passSyncSemaphores[m_passMetaData.size() - 1];
        }

        auto SetRenderFinishSemaphore(VkSemaphore Semaphore) {
            m_renderFinishSemaphore = Semaphore;
        }

    private:
        std::vector<RenderPassMetaData> m_passMetaData;
        std::vector<VkSemaphore> m_passSyncSemaphores;
        uint m_frameIndex = 0;
        VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
        VkFence m_renderFinish = VK_NULL_HANDLE;
        VkSemaphore m_renderFinishSemaphore = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RENDERGRAPH_H
