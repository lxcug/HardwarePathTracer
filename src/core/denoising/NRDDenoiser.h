//
// Created by HUSTLX on 2025/2/12.
//

#ifndef SHADOWY_NRDDENOISER_H
#define SHADOWY_NRDDENOISER_H

#include "NRD.h"
#include "NRI.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"
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
