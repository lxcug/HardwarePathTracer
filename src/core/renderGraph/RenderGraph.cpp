//
// Created by HUSTLX on 2024/10/18.
//

#include "RenderGraph.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {
    RenderGraph::RenderGraph() {
        Create();
    }

    RenderGraph::~RenderGraph() {
        Destroy();
    }

    void RenderGraph::Create() {}

    void RenderGraph::Destroy() {
//        for (auto &PassMetaData: m_passMetaData) {
//            delete PassMetaData.Pass;
//        }
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

    auto RenderGraph::Validate() -> bool {
        for (auto &PassMetaData: m_passMetaData) {
            if (PassMetaData.Pass->GetPassFlag() == PassFlag::Raster) {
                auto Raster = dynamic_cast<RasterPass *>(PassMetaData.Pass);
                // Make sure BasePass is raster first(cause only base pass do attachments clear ops)
                if (Raster->IsBasePass()) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    void RenderGraph::OnNewFrame() {
        Destroy();
    }

    void RenderGraph::Execute() {
        if (!Validate()) {
            throw std::runtime_error("RenderGraph Validate Fails");
        }

        m_passSyncSemaphores.resize(m_passMetaData.size());
        VkSemaphoreCreateInfo SemaphoreInfo{};
        SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (int i = 0; i < m_passSyncSemaphores.size(); i++) {
            vkCreateSemaphore(GetVKDevice(), &SemaphoreInfo, nullptr, &m_passSyncSemaphores[i]);
        }

        VkCommandBuffer LastCommandBuffer = VK_NULL_HANDLE;
        uint PassIndex = 0;
        for (auto &PassMetaData: m_passMetaData) {
            auto CurrentCommandBuffer = PassMetaData.Pass->GetCurrentCommandBuffer();

            if (CurrentCommandBuffer == LastCommandBuffer) {
                vkDeviceWaitIdle(GetVKDevice());
            }
            LastCommandBuffer = CurrentCommandBuffer;

            vkResetCommandBuffer(CurrentCommandBuffer, 0);
            VkCommandBufferBeginInfo BeginInfo{};
            BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            VK_CHECK(vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo));
            PassMetaData.ExecLambda();
            VK_CHECK(vkEndCommandBuffer(CurrentCommandBuffer));

            VkSubmitInfo PassSubmitInfo{};
            PassSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            if (PassIndex == 0) {  // Wait for swapchain image available
                PassSubmitInfo.waitSemaphoreCount = 1;
                PassSubmitInfo.pWaitSemaphores = &m_imageAvailable;
                VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                PassSubmitInfo.pWaitDstStageMask = &WaitStage;
            } else {  // wait for last pass finish
                PassSubmitInfo.waitSemaphoreCount = 1;
                PassSubmitInfo.pWaitSemaphores = &m_passSyncSemaphores[PassIndex - 1];
                VkPipelineStageFlags WaitStage =
                        PassMetaData.Pass->GetPassFlag() == PassFlag::Raster ?
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                                                             : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                PassSubmitInfo.pWaitDstStageMask = &WaitStage;
            }

            if (PassIndex == m_passMetaData.size() - 1) {
                PassSubmitInfo.signalSemaphoreCount = 2;
                std::array<VkSemaphore, 2> SignalSemaphores = {
                        m_passSyncSemaphores[PassIndex],
                        m_renderFinishSemaphore
                };
                PassSubmitInfo.pSignalSemaphores = SignalSemaphores.data();
            } else {
                PassSubmitInfo.signalSemaphoreCount = 1;
                PassSubmitInfo.pSignalSemaphores = &m_passSyncSemaphores[PassIndex];
            }
            PassSubmitInfo.commandBufferCount = 1;
            PassSubmitInfo.pCommandBuffers = &CurrentCommandBuffer;

            auto Queue = PassMetaData.Pass->GetPassFlag() == PassFlag::Raster ?
                         VulkanBackendApp::GetApplication()->GetGlobalQueue().GraphicsQueue :
                         VulkanBackendApp::GetApplication()->GetGlobalQueue().ComputeQueue;
            VK_CHECK(vkQueueSubmit(Queue, 1, &PassSubmitInfo,
                                   PassIndex == m_passMetaData.size() - 1 ? m_renderFinish
                                                                          : nullptr));
            PassIndex++;
        }
    }
}  // namespace HWPT
