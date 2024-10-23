//
// Created by HUSTLX on 2024/10/23.
//

#include "Scene.h"


namespace HWPT {

    Scene::~Scene() {
        for (auto& Primitive : m_primitives) {
            delete Primitive;
        }
    }

    void Scene::AddPrimitive(Model *Primitive) {
        m_primitives.push_back(Primitive);
    }
}  // namespace HWPT
