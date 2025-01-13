//
// Created by HUSTLX on 2025/1/5.
//

#ifndef HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H
#define HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H

#include "VulkanBackendApp.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "core/acceleration_structure/AccelerationStructure.h"
#include "core/scene/Scene.h"


namespace HWPT
{
    class VulkanRayTracingApp : public VulkanBackendApp
    {
    public:
        VulkanRayTracingApp();

        void InitVulkan() override;

        void DrawFrame() override;

        void CreateSyncObjects() override;

        void CleanUp() override;

        void DrawImGuiFrame() override;

        void OnWindowResize() override;

    protected:
        void InitRayTracing();

        void CreateRTDescriptorSets();

        void BindRTDescriptorSets();

        void CreateRTPipelineLayout();

        void CreateRTSBT();

        void CreateRTPipeline();

        void CreateAccelerationStructure();

        void CreateViewportImages();

        void ResizeViewportImages();

        VkDescriptorSetLayout m_RTDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_RTDescriptorSets;
        VkPipelineLayout m_RTPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_RTPipeline = VK_NULL_HANDLE;

        enum StageIndices : uint8_t
        {
            RayGen,
            Miss,
            ClosestHit,
            AnyHit,
            Intersection,
            StageIndicesMax
        };

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;

        ArbitraryBuffer* m_RTSBTBuffer = nullptr;
        VkStridedDeviceAddressRegionKHR m_rayGenRegion{};
        VkStridedDeviceAddressRegionKHR m_missRegion{};
        VkStridedDeviceAddressRegionKHR m_hitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProps{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };

        Texture2D* m_lastFrameSceneColor = nullptr;
        Scene* m_RTScene = nullptr;

        std::vector<Texture2D*> m_viewportImages;
        Texture2D* m_lastFrameViewportImage = nullptr;

        glm::vec2 m_viewportSize = glm::vec2(1.f, 1.f);
        glm::vec2 m_viewportOffset = glm::vec2(0.f, 0.f);
        bool m_shouldRecreateViewportImages = false;
        std::vector<VkDescriptorSet> m_viewportImageDescriptorSets;
    };
} // namespace HWPT

#endif //HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H
