//
// Created by HUSTLX on 2025/1/10.
//

#include "Scene.h"
#include "core/RHI.h"


namespace HWPT {

    void Scene::CreateAccel() {
        m_accelBuilder = std::make_shared<ASBuilder>();

        std::vector<BLASBuildInput> BLASBuildVector;
        std::vector<VkAccelerationStructureInstanceKHR> TLASBuildVector;
        BLASBuildVector.reserve(m_models.size());
        TLASBuildVector.reserve(m_models.size());

        for (const auto &Model_: m_models) {
            BLASBuildVector.emplace_back(Model_->GetBLASBuildInput());
        }
        m_accelBuilder->BuildBLAS(BLASBuildVector);

        for (const auto &Model_: m_models) {
            TLASBuildVector.emplace_back(Model_->GetTLASBuildInput(m_accelBuilder.get()));
        }
        m_accelBuilder->BuildTLAS(TLASBuildVector);
    }

    void Scene::CreateModelDescBuffer() {
        m_sceneModelDescBuffer = std::make_shared<ArbitraryBuffer>(
                sizeof(ModelDesc) * m_modelDescs.size(),
                m_modelDescs.data(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    void Scene::CreateModelTextures(const std::vector<std::string> &TexturePaths) {
        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer();
        for (auto &TexturePath: TexturePaths) {
            auto SharedTexture = std::make_shared<Texture2D>(TexturePath, 1, false);
            RHI::TextureTransitionInput SrcInput{}, DstInput{};
            SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            SrcInput.AccessMask = 0;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DstInput.AccessMask = VK_ACCESS_SHADER_READ_BIT;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            RHI::TransitionTextureLayout(CommandBuffer, SharedTexture->GetHandle(), 1, SrcInput,
                                         DstInput);
            m_sceneModelTextures.emplace_back(SharedTexture);
        }
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer);
    }
}  // namespace HWPT
