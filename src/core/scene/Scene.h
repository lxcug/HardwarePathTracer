//
// Created by HUSTLX on 2025/1/10.
//

#ifndef HARDWAREPATHTRACER_SCENE_H
#define HARDWAREPATHTRACER_SCENE_H

#include "core/Core.h"
#include <vector>
#include "Model.h"
#include "host_device_shared/Light.h"


namespace Shadowy {
    class ASBuilder;

    class Scene {
    public:
        Scene() = default;

        ~Scene();

        void AddModel(Model *ModelPtr) {
            std::shared_ptr<Model> SharedModel(ModelPtr);
            AddModel(SharedModel);
        }

        template<typename... Args>
        void AddModel(const std::string &ModelName, Args &&... Args_) {
            auto SharedModel = std::make_shared<Model>(Args_...);
            SharedModel->SetModelName(ModelName);
            AddModel(SharedModel);
        }

        template<typename... Args>
        void AddModel(Args &&... Args_) {
            auto SharedModel = std::make_shared<Model>(Args_...);
            AddModel(SharedModel);
        }

        void AddModel(std::shared_ptr<Model> &SharedModel) {
            SharedModel->SetInstanceID(s_instanceIDCounter++);
            // NOTE: Set Global Texture Offset for each ModelDesc
            SharedModel->SetModelDescTextureOffset(static_cast<int>(m_sceneModelTextures.size()));
            CreateModelTextures(SharedModel->GetModelTexturePaths());

            m_models.emplace_back(SharedModel);
            m_modelDescs.emplace_back(SharedModel->GetModelDesc());
        }

        template<typename... Args>
        void AddLight(Args &&... Args_) {
            m_sceneLights.emplace_back(Light{Args_...});
        }

        void AddLight(const Light& Light) {
            m_sceneLights.emplace_back(Light);
        }

        void FinalizeScene();

        void CreateAccel();

        void AppendSkyLightToSceneLights();

        auto GetAccelBuilder() -> std::shared_ptr<ASBuilder> & {
            return m_accelBuilder;
        }

        [[nodiscard]] auto
        GetSceneModelTextures() const -> const std::vector<std::shared_ptr<Texture2D>> & {
            return m_sceneModelTextures;
        }

        void CreateModelDescBuffer();

        auto GetModelDescBuffer() -> std::shared_ptr<ArbitraryBuffer> & {
            return m_sceneModelDescBuffer;
        }

        void CreateModelTextures(const std::vector<std::string> &TexturePaths);

        void CreateSceneDescriptorSet();

        void OnRecreate();

        void UpdateSceneDescDescriptorSets() const;

        [[nodiscard]] auto GetSceneDescriptorSetLayout() const -> VkDescriptorSetLayout {
            return m_sceneDescDescriptorSetLayout;
        }

        [[nodiscard]] auto GetModelDescDescriptorSet(uint ImageIndex) const -> VkDescriptorSet {
            Check(ImageIndex < MAX_FRAMES_IN_FLIGHT);
            return m_sceneDescDescriptorSets[ImageIndex];
        }

        void CreateSceneLightsBuffer();

        [[nodiscard]] auto
        GetSceneLightsBuffer() const -> const std::shared_ptr<ArbitraryBuffer> & {
            return m_sceneLightsBuffer;
        }

        auto GetSceneLights() -> std::vector<Light> & {
            return m_sceneLights;
        }

        void CreateSkyTexture(const std::filesystem::path &Path) {
            // NOTE: SRGB
            m_skyTexture = std::make_shared<Texture2D>(Path, 1, false, true);
        }

    private:
        std::shared_ptr<ASBuilder> m_accelBuilder;
        std::vector<std::shared_ptr<Model>> m_models;
        std::vector<ModelDesc> m_modelDescs;
        std::shared_ptr<ArbitraryBuffer> m_sceneModelDescBuffer;
        static inline uint s_instanceIDCounter = 0; // TODO: dispatch instance index to models
        std::vector<std::shared_ptr<Texture2D>> m_sceneModelTextures;

        VkDescriptorSetLayout m_sceneDescDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_sceneDescDescriptorSets;

        std::vector<Light> m_sceneLights;
        std::shared_ptr<ArbitraryBuffer> m_sceneLightsBuffer;

        std::shared_ptr<Texture2D> m_skyTexture;

    private:
        Light m_dummyLight;
        ModelDesc m_dummyDesc;
        // Used when m_models.empty()
        std::shared_ptr<ArbitraryBuffer> m_sceneModelDescDummyBuffer;
        // Used when m_sceneLights.empty()
        std::shared_ptr<ArbitraryBuffer> m_sceneLightsDummyBuffer;
    };
} // namespace Shadowy

#endif //HARDWAREPATHTRACER_SCENE_H
