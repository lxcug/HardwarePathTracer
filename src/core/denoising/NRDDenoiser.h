//
// Created by HUSTLX on 2025/2/12.
//

#ifndef SHADOWY_NRDDENOISER_H
#define SHADOWY_NRDDENOISER_H

#include "NRDIntegration.h"


namespace Shadowy {

    class NRDDenoiser {
    public:
        void SetResources();

        void OnResize() {
            m_integration.Destroy();
//            m_integration.Initialize();
        }

    private:
        nrd::Integration m_integration;
    };
}  // namespace Shadowy

#endif //SHADOWY_NRDDENOISER_H
