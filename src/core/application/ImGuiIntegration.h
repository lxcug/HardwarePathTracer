//
// Created by HUSTLX on 2024/10/13.
//

#ifndef HARDWAREPATHTRACER_IMGUIINTEGRATION_H
#define HARDWAREPATHTRACER_IMGUIINTEGRATION_H

#include "core/Core.h"
#include <vector>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"


namespace HWPT {
    class ImGuiInfrastructure {
    public:
        ImGuiInfrastructure(uint NumFrames);  // NOLINT

        void RecreateFrameBuffer();

        ~ImGuiInfrastructure();

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
//        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
//        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkFramebuffer> m_frameBuffers;

        void BeginImGui();

        void EndImGui();

        void EndImGui(VkCommandBuffer CommandBuffer);

        static void Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
            ImGui::SetNextWindowBgAlpha(m_imguiUIAlpha);
            ImGui::Begin(name, p_open, flags);
        }

        static void End() {
            ImGui::End();
        }

        static void SetWindowAlpha(float Alpha) {
            m_imguiUIAlpha = Alpha;
        }

    private:
        uint m_numFrames = 0;
        inline static uint MaxNumTextures = 10;
        inline static float m_imguiUIAlpha = 0.5;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_IMGUIINTEGRATION_H
