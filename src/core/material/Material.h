//
// Created by HUSTLX on 2025/1/12.
//

#ifndef HARDWAREPATHTRACER_MATERIAL_H
#define HARDWAREPATHTRACER_MATERIAL_H

#include "core/Core.h"


namespace HWPT {
    struct Material {
        glm::vec3 Albedo = glm::vec3(1.f, 1.f, 1.f);
        int AlbedoTextureID = -1;

        glm::vec3 Emissive = glm::vec3(0.f, 0.f, 0.f);
        int EmissiveTextureID = -1;

        glm::vec3 Transmittance = glm::vec3(1.f, 1.f, .1f);
        float Opacity = 1.f;

        float Roughness = 1.f;
        float Metallic = 0.f;
        int RoughnessTextureID = -1;
        int MetallicTextureID = -1;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_MATERIAL_H
