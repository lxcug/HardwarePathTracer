//
// Created by HUSTLX on 2024/10/18.
//

#include "RenderGraph.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {
    RenderGraph::RenderGraph(uint NumConcurrentFrames) : m_concurrentFrames(NumConcurrentFrames) {
        Create();
        m_frameImageAvailable.resize(m_concurrentFrames);
        m_frameImageRenderFinish.resize(m_concurrentFrames);
        VkSemaphoreCreateInfo SemaphoreInfo{};
        SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo FenceInfo{};
        FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        for (int i = 0; i < m_concurrentFrames; i++) {
            VK_CHECK(vkCreateSemaphore(GetVKDevice(), &SemaphoreInfo, nullptr,
                                       &m_frameImageAvailable[i]));
            VK_CHECK(vkCreateSemaphore(GetVKDevice(), &SemaphoreInfo, nullptr,
                                       &m_frameImageRenderFinish[i]));
        }
    }

    RenderGraph::~RenderGraph() {
        for (auto &PassMetaData: m_passMetaData) {
            delete PassMetaData.Pass;
        }
        Destroy();
        for (int i = 0; i < m_concurrentFrames; i++) {
            vkDestroySemaphore(GetVKDevice(), m_frameImageAvailable[i], nullptr);
            vkDestroySemaphore(GetVKDevice(), m_frameImageRenderFinish[i], nullptr);
        }
    }

    void RenderGraph::Create() {}

    void RenderGraph::Destroy() {
        m_passMetaData.clear();
        for (int i = 0; i < m_passSyncSemaphores.size(); i++) {
            vkDestroySemaphore(GetVKDevice(), m_passSyncSemaphores[i], nullptr);
        }
        m_passSyncSemaphores.clear();
    }

    void RenderGraph::AddPass(const std::string &PassName, RenderPassBase *Pass,
                              const std::function<void()> &ExecLambda) {
        RenderPassMetaData PassMetaData{PassName, Pass, ExecLambda};
        m_passMetaData.push_back(PassMetaData);
    }

    auto RenderGraph::AllocateParameters(RenderPassBase *Pass,
                                         const std::initializer_list<std::pair<std::string, ShaderParameterType>> &InitList) -> ShaderParameters * {
        auto *ShaderParameter = new ShaderParameters(Pass, InitList);
        return ShaderParameter;
    }

    auto RenderGraph::Validate() -> bool {
        if (m_passMetaData.empty()) {
            return false;
        }

        // Make sure BasePass is raster first(cause only base pass do attachments clear ops)
        for (auto &PassMetaData: m_passMetaData) {
            if (PassMetaData.Pass->GetPassFlag() == PassFlag::Raster) {
                auto Raster = dynamic_cast<RasterPass *>(PassMetaData.Pass);
                if (Raster->IsBasePass()) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    void RenderGraph::OnNewFrame(uint FrameIndex) {
        Destroy();
        m_frameIndex = FrameIndex;
    }

    void RenderGraph::Execute() {
        if (!Validate()) {
            throw std::runtime_error("RenderGraph Validate Fails");
        }

        m_passSyncSemaphores.resize(m_passMetaData.size() - 1);
        VkSemaphoreCreateInfo SemaphoreInfo{};
        SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (int i = 0; i < m_passSyncSemaphores.size(); i++) {
            vkCreateSemaphore(GetVKDevice(), &SemaphoreInfo, nullptr, &m_passSyncSemaphores[i]);
        }

        VkCommandBuffer LastCommandBuffer = VK_NULL_HANDLE;

        for (int i = 0; i < m_passMetaData.size(); i++) {
            auto &PassMetaData = m_passMetaData[i];
            auto &Pass = PassMetaData.Pass;

            bool IsFirstPass = i == 0;
            bool IsLastPass = i == m_passMetaData.size() - 1;
            bool IsRasterPass = Pass->GetPassFlag() == PassFlag::Raster;
            bool IsComputePass = Pass->GetPassFlag() == PassFlag::Compute ||
                                 Pass->GetPassFlag() == PassFlag::AsyncCompute;
            // Wait CommandBuffer Free TODO: Merge 2 Pass into SameCommandBuffer(Queue or CommandBuffer)
            auto CurrentCommandBuffer = Pass->GetCurrentCommandBuffer();
            bool UseSameCommandBufferWithNextPass = false;
            if (!IsLastPass &&
                CurrentCommandBuffer == m_passMetaData[i + 1].Pass->GetCurrentCommandBuffer()) {
                UseSameCommandBufferWithNextPass = true;
            }

            bool UseSameCommandBufferWithLastPass = CurrentCommandBuffer == LastCommandBuffer;
            LastCommandBuffer = CurrentCommandBuffer;

            // TODO: Make sure no dependency when merge commands
            // Begin CommandBuffer(don't begin when use the same command buffer with last pass for command merge)
            if (!UseSameCommandBufferWithLastPass) {
                vkResetCommandBuffer(CurrentCommandBuffer, 0);
                VkCommandBufferBeginInfo BeginInfo{};
                BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                VK_CHECK(vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo));
            }

//            Pass->BindRenderPass(CurrentCommandBuffer);
            PassMetaData.ExecLambda();

            // End CommandBuffer(don't end command buffer and submit commands when use the same command buffer with next pass for command merge)
            if (!UseSameCommandBufferWithNextPass) {
                VK_CHECK(vkEndCommandBuffer(CurrentCommandBuffer));

                // Submit Commands
                VkSubmitInfo PassSubmitInfo{};
                PassSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                if (IsFirstPass) {  // Wait for swapchain image available
                    PassSubmitInfo.waitSemaphoreCount = 1;
                    PassSubmitInfo.pWaitSemaphores = &m_frameImageAvailable[m_frameIndex];
                    VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    PassSubmitInfo.pWaitDstStageMask = &WaitStage;
                } else {  // wait for last pass finish
                    PassSubmitInfo.waitSemaphoreCount = 1;
                    PassSubmitInfo.pWaitSemaphores = &m_passSyncSemaphores[i - 1];
                    VkPipelineStageFlags WaitStage =
                            Pass->GetPassFlag() == PassFlag::Raster
                            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                            : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    PassSubmitInfo.pWaitDstStageMask = &WaitStage;
                }
                if (UseSameCommandBufferWithLastPass) {
                    PassSubmitInfo.waitSemaphoreCount = 0;
                }

                PassSubmitInfo.signalSemaphoreCount = 1;
                PassSubmitInfo.pSignalSemaphores = IsLastPass ? &m_frameImageRenderFinish[m_frameIndex]
                                                              : &m_passSyncSemaphores[i];
                PassSubmitInfo.commandBufferCount = 1;
                PassSubmitInfo.pCommandBuffers = &CurrentCommandBuffer;

                auto Queue = Pass->GetPassFlag() == PassFlag::Raster ?
                             VulkanBackendApp::GetApplication()->GetGlobalQueue().GraphicsQueue :
                             VulkanBackendApp::GetApplication()->GetGlobalQueue().ComputeQueue;
                VK_CHECK(vkQueueSubmit(Queue, 1, &PassSubmitInfo, nullptr));
            }
        }
    }
}  // namespace HWPT
