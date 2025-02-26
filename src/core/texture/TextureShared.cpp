//
// Created by HUSTLX on 2024/10/12.
//

#include "TextureShared.h"


namespace Shadowy {
    auto GetVKSampleCount(uint SampleCount) -> VkSampleCountFlagBits {
        switch (SampleCount) {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 2:
                return VK_SAMPLE_COUNT_2_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            case 8:
                return VK_SAMPLE_COUNT_8_BIT;
            case 16:
                return VK_SAMPLE_COUNT_16_BIT;
            case 32:
                return VK_SAMPLE_COUNT_32_BIT;
            case 64:
                return VK_SAMPLE_COUNT_64_BIT;
            default:
                Check(false);
                return VK_SAMPLE_COUNT_1_BIT;
        }
    }
}  // namespace Shadowy
