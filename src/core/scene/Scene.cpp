//
// Created by HUSTLX on 2025/1/10.
//

#include "Scene.h"
#include "core/RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace Shadowy {
    Scene::~Scene() {
        vkDestroyDescriptorSetLayout(GetVKDevice(), m_sceneDescDescriptorSetLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             m_sceneDescDescriptorSets.size(), m_sceneDescDescriptorSets.data());
    }

    void Scene::CreateAccel() {
        m_accelBuilder = std::make_shared<ASBuilder>();
        m_accelBuilder->ResetBLASIndex();

        std::vector<BLASBuildInput> BLASBuildVector;
        std::vector<VkAccelerationStructureInstanceKHR> TLASBuildVector;

        for (auto& Model : m_models)
        {
            for (auto& Mesh : Model->m_meshes)
            {
                BLASBuildVector.emplace_back(Mesh->GetBLASBuildInput());
            }
        }
        m_accelBuilder->BuildBLAS(BLASBuildVector);

        for (auto& Model : m_models)
        {
            for (auto& Mesh : Model->m_meshes)
            {
                TLASBuildVector.emplace_back(Mesh->GetTLASBuildInput(m_accelBuilder.get()));
            }
        }
        m_accelBuilder->BuildTLAS(TLASBuildVector);
    }

    void Scene::CreateModelDescBuffer() {
        if (!m_modelDescs.empty()) {
            m_sceneModelDescBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(ModelDesc) * m_modelDescs.size(),
                    m_modelDescs.data(),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        } else {
            m_sceneModelDescDummyBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(ModelDesc), &m_dummyDesc,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
    }

    // void Scene::CreateModelTextures(Model* Model) {
    //     auto CommandBuffer = RHI::BeginIntermediateCommandBuffer();
    //
    //     for (auto &TexturePath : Model->m_textureNames) {
    //         // Handle Embedded Texture
    //         const aiTexture* TextureData = Model->m_aiScene->GetEmbeddedTexture(TexturePath.c_str());
    //         std::shared_ptr<Texture2D> SharedTexture;
    //         if (TextureData)
    //         {
    //             SharedTexture = std::make_shared<Texture2D>(TextureData, false, false);
    //         }
    //         else
    //         {
    //             std::string FullPath = Model->m_path.string() + '/' + TexturePath;
    //             // SharedTexture = std::make_shared<Texture2D>(FullPath, false, false);
    //
    //             auto [TexPtr, TexIndex] = m_textureManager->CreateOrRetrieveTexture(FullPath);
    //         }
    //
    //         RHI::TextureTransitionInput SrcInput{}, DstInput{};
    //         SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    //         SrcInput.AccessMask = 0;
    //         DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
    //         DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //         DstInput.AccessMask = 0;
    //         DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
    //         RHI::TransitionTextureLayout(CommandBuffer, SharedTexture->GetHandle(), 1, SrcInput,
    //                                      DstInput);
    //         m_sceneModelTextures.emplace_back(SharedTexture);
    //     }
    //     RHI::SubmitIntermediateCommandBuffer(CommandBuffer);
    // }

    void Scene::CreateSceneDescriptorSet() {
        VkDescriptorSetLayoutBinding ModelInfoBinding{};
        ModelInfoBinding.binding = 0;
        ModelInfoBinding.descriptorCount = 1;
        ModelInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ModelInfoBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding TexturesBinding{};
        TexturesBinding.binding = 1;
        TexturesBinding.descriptorCount = m_textureManager->m_uniqueTextures.size();
        TexturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        TexturesBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                     VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding SceneLightsBinding{};
        SceneLightsBinding.binding = 2;
        SceneLightsBinding.descriptorCount = 1;
        SceneLightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        SceneLightsBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding SkyTextureBinding{};
        SkyTextureBinding.binding = 3;
        SkyTextureBinding.descriptorCount = 1;
        SkyTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SkyTextureBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        std::array<VkDescriptorSetLayoutBinding, 4> Bindings = {
                ModelInfoBinding, TexturesBinding, SceneLightsBinding, SkyTextureBinding
        };

        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = Bindings.size();
        CreateInfo.pBindings = Bindings.data();
        CreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &CreateInfo, nullptr,
                                             &m_sceneDescDescriptorSetLayout));

        std::vector<VkDescriptorSetLayout> Layouts(
                MAX_FRAMES_IN_FLIGHT, m_sceneDescDescriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetDescriptorPool();
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_sceneDescDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(
                vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo,
                                         m_sceneDescDescriptorSets.data()
                ));
    }

    void Scene::UpdateSceneDescDescriptorSets() const {
        std::array<VkWriteDescriptorSet, 4> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo ModelDescBufferInfo{};
            ModelDescBufferInfo.buffer = m_models.empty()
                                         ? m_sceneModelDescDummyBuffer->GetHandle()
                                         : m_sceneModelDescBuffer->GetHandle();
            ModelDescBufferInfo.offset = 0;
            ModelDescBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_sceneDescDescriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[0].pBufferInfo = &ModelDescBufferInfo;

            auto& SceneModelTextures = m_textureManager->m_uniqueTextures;
            std::vector<VkDescriptorImageInfo> TexturesInfos(SceneModelTextures.size());
            for (int index = 0; index < SceneModelTextures.size(); index++) {
                VkDescriptorImageInfo TextureInfo{};
                TextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                TextureInfo.imageView = SceneModelTextures[index]->CreateSRV();
                TextureInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
                TexturesInfos[index] = TextureInfo;
            }
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_sceneDescDescriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = SceneModelTextures.size();
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrites[1].pImageInfo = TexturesInfos.data();

            VkDescriptorBufferInfo SceneLightsBufferInfo{};
            SceneLightsBufferInfo.buffer = m_sceneLights.empty()
                                           ? m_sceneLightsDummyBuffer->GetHandle()
                                           : m_sceneLightsBuffer->GetHandle();
            SceneLightsBufferInfo.offset = 0;
            SceneLightsBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_sceneDescDescriptorSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[2].pBufferInfo = &SceneLightsBufferInfo;

            VkDescriptorImageInfo SkyTextureInfo{};
            SkyTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            SkyTextureInfo.imageView = m_skyTexture->CreateSRV();
            SkyTextureInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].dstSet = m_sceneDescDescriptorSets[i];
            DescriptorWrites[3].dstBinding = 3;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrites[3].pImageInfo = &SkyTextureInfo;

            vkUpdateDescriptorSets(GetVKDevice(), DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

    void Scene::CreateSceneLightsBuffer() {
        if (!m_sceneLights.empty()) {
            for (auto& Light : m_sceneLights) {
                Light.Direction = glm::normalize(Light.Direction);
            }
            m_sceneLightsBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(Light) * m_sceneLights.size(),
                    m_sceneLights.data(),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        } else {
            m_sceneLightsDummyBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(Light), &m_dummyLight,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
    }

    void Scene::FinalizeScene() {
        CreateAccel();
        CreateModelDescBuffer();
        AppendSkyLightToSceneLights();
        CreateSceneLightsBuffer();
        if (m_textureManager->m_uniqueTextures.empty()) {
            m_textureManager->m_uniqueTextures.emplace_back(
                    new Texture2D(1, 1,
                        VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_SAMPLED_BIT));
            RHI::TextureTransitionInput SrcInput{}, DstInput{};
            SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            SrcInput.AccessMask = 0;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DstInput.AccessMask = 0;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            RHI::TransitionTextureLayout(m_textureManager->m_uniqueTextures[0]->GetHandle(), 1, SrcInput,
                                         DstInput);
        }
        CreateSceneDescriptorSet();
        UpdateSceneDescDescriptorSets();
    }

    void Scene::OnRecreate() {
        vkDestroyDescriptorSetLayout(GetVKDevice(), m_sceneDescDescriptorSetLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             m_sceneDescDescriptorSets.size(), m_sceneDescDescriptorSets.data());
        m_accelBuilder.reset();
        m_models.clear();
        m_modelDescs.clear();
        m_sceneModelDescBuffer.reset();
        m_textureManager->m_uniqueTextures.clear();
        m_textureManager->m_storedTextures.clear();
        s_instanceIDCounter = 0;
        m_sceneLights.erase(m_sceneLights.end() - 1);
    }

    void Scene::AppendSkyLightToSceneLights() {
        if (m_skyTexture) {
            Light SkyLight{
                    glm::vec3(0.f, 0.f, 0.f),
                    LightType::Sky,
                    glm::vec3(0.f, 0.f, 0.f),
                    0.f,
                    float3(1.f, 1.f, 1.f),
                    1.f,
                    0.f,
                    0.f,
                    0.f
            };
            m_sceneLights.push_back(SkyLight);
        }
    }
} // namespace Shadowy
