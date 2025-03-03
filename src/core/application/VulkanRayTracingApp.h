//
// Created by HUSTLX on 2025/1/5.
//

#ifndef HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H
#define HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H

#include "VulkanBackendApp.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "core/acceleration_structure/AccelerationStructure.h"
#include "core/scene/Scene.h"
#include "core/gbuffer/GBufferPass.h"
#include "host_device_shared/Light.h"
#include "host_device_shared/RenderOptions.h"
#include "core/post_processing/ToneMapping.h"
#include "core/restir/ReSTIRResource.h"
#include "GlobalTexture.h"
#include "core/post_processing/AccumulationPass.h"
#include "core/post_processing/BloomPass.h"


namespace Shadowy {
    class VulkanRayTracingApp : public VulkanBackendApp {
    public:
        explicit VulkanRayTracingApp(const std::string &Title = "Shadowy");

        void Run() override;

        void ResetFrameNum() override
        {
            m_accumulatedFrameNum = 0;
            m_currentAccumulatedRenderTime = 0.f;
        }

        void InitVulkan() override;

        void RenderFrame() override;

        void CreateSyncObjects() override;

        void CleanUp() override;

        void DrawImGuiFrame() override;

        void OnWindowResize() override;

        void UpdateRenderOptions();

        auto GetScene() const -> Scene* {
            return m_RTScene;
        };

        auto GetLastFrameSceneColor() const -> Texture2D* {
            return m_lastFrameViewportImage;
        }

    protected:
        void InitRayTracing();

        void CreateRTDescriptorSets();

        void UpdateRTDescriptorSets();

        void CreateRTPipelineLayout();

        void CreateRTSBT();

        void CreateRTPipeline();

        void ReloadScene(const std::filesystem::path &Path);

        void InitScene();

        void CreateViewportImages();

        void ResizeViewportImages();

        void ReCompileShaders();

        VkDescriptorSetLayout m_RTDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_RTDescriptorSets;
        VkPipelineLayout m_RTPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_RTPipeline = VK_NULL_HANDLE;

        enum StageIndices : uint8_t {
            RayGen,
            Miss,
            ClosestHit,
            AnyHit,
            Intersection,
            StageIndicesMax
        };
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;
        ArbitraryBuffer *m_RTSBTBuffer = nullptr;
        VkStridedDeviceAddressRegionKHR m_rayGenRegion{};
        VkStridedDeviceAddressRegionKHR m_missRegion{};
        VkStridedDeviceAddressRegionKHR m_hitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProps{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };

        Scene *m_RTScene = nullptr;

        Texture2D *m_lastFrameViewportImage = nullptr;

        glm::vec2 m_viewportSize = glm::vec2(1.f, 1.f);
        glm::vec2 m_currentViewportImageSize = m_viewportSize;
        glm::vec2 m_viewportOffset = glm::vec2(0.f, 0.f);
        std::vector<VkDescriptorSet> m_viewportImageDescriptorSets;
        PathTracingOptions m_pathTracingOptions;
        float m_currentAccumulatedRenderTime = 0.f;  // Seconds
        float m_maxRenderTime = 0.f;  // <= 0 means infinite accumulation

        // NOTE: Store Last Frame Operations and Execute before RenderPipeline Begins
        std::vector<std::function<void()>> m_deferredOperations;

        bool m_enableBloom = false;
        ToneMappingPass *m_toneMappingPass = nullptr;

        // ReSTIRResource* m_restirResource = nullptr;
        // Delay several frames for correct accumulation when resize viewport images
        inline static int s_numFramesToDelay = 3;
        int m_currentDelayFrames = 0;

        // Post Processing
        GBufferPass* m_gbufferPass = nullptr;
        AccumulationPass* m_accumulationPass = nullptr;
        BloomPass* m_bloomPass = nullptr;

    public:
        GlobalTexture* m_globalTexture = nullptr;
    };
} // namespace Shadowy

#endif //HARDWAREPATHTRACER_VULKANRAYTRACINGAPP_H
