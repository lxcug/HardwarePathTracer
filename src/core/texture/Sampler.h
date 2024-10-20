//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_SAMPLER_H
#define HARDWAREPATHTRACER_SAMPLER_H

#include "core/Core.h"


namespace HWPT {
    class Sampler {
    public:
        Sampler();

        ~Sampler();

        void CreateSampler();

        auto GetHandle() -> VkSampler& {
            return m_sampler;
        }

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SAMPLER_H
