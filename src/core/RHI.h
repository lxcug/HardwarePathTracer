//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_RHI_H
#define HARDWAREPATHTRACER_RHI_H

#include "core/Core.h"
#include "vulkan/vulkan.h"


// NOTE: Only Support Vulkan, Actually is a Util Funcs Header Now
namespace HWPT::RHI {
    uint FindMemoryType(uint TypeFilter, VkMemoryPropertyFlags Properties);

    void CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties,
                      VkBuffer& Buffer, VkDeviceMemory& BufferMemory);

    void CopyBuffer(VkBuffer Src, VkBuffer Dst, VkDeviceSize Size);

    std::tuple<VkBuffer, VkDeviceMemory> CreateStagingBuffer(VkDeviceSize Size);

    void CreateTexture2D(uint Width, uint Height, VkFormat Format, VkImageUsageFlags Usage,
                         VkImageTiling Tiling, VkImage& Texture, VkDeviceMemory& TextureMemory);

    void TransitionTextureLayout(VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout);

    void CopyBufferToTexture(VkImage Image, VkBuffer Buffer, uint Width, uint Height);
}

#endif //HARDWAREPATHTRACER_RHI_H
