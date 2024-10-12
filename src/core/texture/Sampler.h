//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_SAMPLER_H
#define HARDWAREPATHTRACER_SAMPLER_H

#include "core/Core.h"


namespace HWPT {
    class Sampler {
    public:
        ~Sampler();

        void CreateSampler();

        VkSampler GetHandle() {
            return m_sampler;
        }

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;
    };
}

#endif //HARDWAREPATHTRACER_SAMPLER_H
