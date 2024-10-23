//
// Created by HUSTLX on 2024/10/23.
//

#ifndef HARDWAREPATHTRACER_SCENE_H
#define HARDWAREPATHTRACER_SCENE_H

#include "core/Model.h"
#include "core/renderGraph/RenderPassCommon.h"


namespace HWPT {
    class Scene {
    public:
        Scene() = default;

        ~Scene();

        void AddPrimitive(Model *Primitive);

        template<typename ...Args>
        void AddPrimitive(Args &&...args) {
            auto Primitive = new Model(std::forward<Args>(args)...);
            m_primitives.push_back(Primitive);
        }

        auto Begin() -> std::vector<Model*>::iterator {
            return m_primitives.begin();
        }

        auto End() -> std::vector<Model*>::iterator {
            return m_primitives.end();
        }

    private:
        std::vector<Model *> m_primitives;
    };

}  // namespace HWPT


#endif //HARDWAREPATHTRACER_SCENE_H
