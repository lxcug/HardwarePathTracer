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

        [[nodiscard]] auto GetHandle() const -> VkSampler {
            return m_sampler;
        }

        static auto GetDefaultSample() -> Sampler*
        {
            static class Sampler* Sampler = new class Sampler;
            return Sampler;
        };

        static void ReleaseSamplers()
        {
            delete GetDefaultSample();
        }

    private:
        VkSampler m_sampler = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SAMPLER_H
