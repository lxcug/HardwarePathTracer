//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURESHARED_H
#define HARDWAREPATHTRACER_TEXTURESHARED_H

#include "core/Core.h"
#include "stb_image.h"


namespace Shadowy {
    auto GetVKSampleCount(uint SampleCount) -> VkSampleCountFlagBits;
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_TEXTURESHARED_H
