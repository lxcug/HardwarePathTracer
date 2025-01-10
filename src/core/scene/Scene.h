//
// Created by HUSTLX on 2025/1/10.
//

#ifndef HARDWAREPATHTRACER_SCENE_H
#define HARDWAREPATHTRACER_SCENE_H

#include "core/Core.h"
#include <vector>
#include "core/Model.h"


namespace HWPT {
    class ASBuilder;

    struct SceneModel {
        std::string ModelName;
        Model ModelInstance;
    };

    class Scene {
    public:
        Scene() = default;

        ~Scene() = default;

        template <typename... Args>
        void AddModel(const std::string& ModelName, Args&&... Args_) {
            Model ModelInstance(std::forward<Args>(Args_)...);
            SceneModel SceneModel_{ModelName, ModelInstance};
            m_models.push_back(std::make_shared<SceneModel>(SceneModel_));
        }

        void FinalizeScene() {
            CreateAccel();
        }

        void CreateAccel();

    private:
        std::shared_ptr<ASBuilder> m_accelBuilder;
        std::vector<std::shared_ptr<SceneModel>> m_models;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SCENE_H
