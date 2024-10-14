//
// Created by HUSTLX on 2024/10/12.
//

#include "Sampler.h"


namespace HWPT {
    Sampler::Sampler() {
        CreateSampler();
    }

    Sampler::~Sampler() {
        vkDestroySampler(GetVKDevice(), m_sampler, nullptr);
    }

    void Sampler::CreateSampler() {
        // Refer: https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/mali-bifrost-usage-recommendations-for-texture-and-sampler-descriptors
        VkSamplerCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        CreateInfo.magFilter = VK_FILTER_LINEAR;
        CreateInfo.minFilter = VK_FILTER_LINEAR;
        CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        CreateInfo.anisotropyEnable = VK_FALSE;
        CreateInfo.maxAnisotropy = 1.f;
        CreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        CreateInfo.unnormalizedCoordinates = VK_FALSE;
        CreateInfo.compareEnable = VK_FALSE;
        CreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        CreateInfo.mipLodBias = 0.f;
        CreateInfo.minLod = 0.f;
        CreateInfo.maxLod = 1000.f;

        VK_CHECK(vkCreateSampler(GetVKDevice(), &CreateInfo, nullptr, &m_sampler));
    }

}  // namespace HWPT
