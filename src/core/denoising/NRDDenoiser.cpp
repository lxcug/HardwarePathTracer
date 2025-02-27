//
// Created by HUSTLX on 2025/2/12.
//

#include "NRDDenoiser.h"

// IMPORTANT: these files must be included beforehand:
#include "NRD.h"
#include "NRI.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"


namespace Shadowy {

    void NRDDenoiser::SetResources() {
        nrd::UserPool UserPool = {};
        // 3D MV RGBA16f+ or 2D MV RG16f+
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_MV, nullptr);
        // RGBA8+   No Encoding!!!
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS, nullptr);
        // Scene Depth R16f+
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_VIEWZ, nullptr);

        // Diffuse RGBAf16+
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST, nullptr);
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST, nullptr);

        // Specular RGBAf16+
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_SPEC_HITDIST, nullptr);
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::OUT_SPEC_HITDIST, nullptr);

        // Sigma
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_PENUMBRA, nullptr);  // R16f+, See SIGMA_FrontEnd_PackPenumbra
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::IN_TRANSLUCENCY, nullptr);  // RGBA8+, See SIGMA_FrontEnd_PackTranslucency
        nrd::Integration_SetResource(UserPool, nrd::ResourceType::OUT_SHADOW_TRANSLUCENCY, nullptr);
    }
}  // namespace Shadowy
