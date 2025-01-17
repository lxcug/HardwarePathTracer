//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_RHI_H
#define HARDWAREPATHTRACER_RHI_H

#include "core/Core.h"
#include "vulkan/vulkan.h"
#include "core/Commands/CommandPool.h"

// NOTE: Only Support Vulkan, Actually is a Util Funcs Header Now
namespace Shadowy::RHI {
    auto FindMemoryType(uint TypeFilter, VkMemoryPropertyFlags Properties) -> uint;

    auto
    BeginIntermediateCommandBuffer(QueueType QueueType_ = QueueType::Graphics) -> VkCommandBuffer;

    // Submit and Wait
    void SubmitIntermediateCommandBuffer(VkCommandBuffer CommandBuffer,
                                         QueueType QueueType = QueueType::Graphics);

    void CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties,
                      VkBuffer &Buffer, VkDeviceMemory &BufferMemory);

    void CopyBuffer(VkBuffer Src, VkBuffer Dst, VkDeviceSize Size);

    auto CreateStagingBuffer(VkDeviceSize Size) -> std::tuple<VkBuffer, VkDeviceMemory>;

    void CreateTexture2D(uint Width, uint Height, uint NumMips, VkSampleCountFlagBits SampleCount,
                         VkFormat Format, VkImageUsageFlags Usage, VkImageTiling Tiling,
                         VkImage &Texture, VkDeviceMemory &TextureMemory);

    void TransitionTextureLayout(VkImage Image, uint NumMips, VkImageLayout OldLayout,
                                 VkImageLayout NewLayout);

    struct TextureTransitionInput {
        VkImageLayout Layout;
        VkAccessFlags AccessMask;
        VkPipelineStageFlags PipelineStage;
    };

    void TransitionTextureLayout(VkImage Image, uint NumMips, VkImageLayout OldLayout,
                                 VkImageLayout NewLayout, VkAccessFlags SrcAccessFlag,
                                 VkAccessFlags DstAccessFlag, VkPipelineStageFlags SrcStage,
                                 VkPipelineStageFlags DstStage);

    void TransitionTextureLayout(VkCommandBuffer CommandBuffer, VkImage Image, uint NumMips,
                                 VkImageLayout OldLayout,
                                 VkImageLayout NewLayout, VkAccessFlags SrcAccessFlag,
                                 VkAccessFlags DstAccessFlag, VkPipelineStageFlags SrcStage,
                                 VkPipelineStageFlags DstStage);

    void TransitionTextureLayout(VkImage Image, uint NumMips,
                                 const TextureTransitionInput &SrcInput,
                                 const TextureTransitionInput &DstInput);

    void TransitionTextureLayout(VkCommandBuffer CommandBuffer, VkImage Image, uint NumMips,
                                 const TextureTransitionInput &SrcInput,
                                 const TextureTransitionInput &DstInput);

    void CopyBufferToTexture(VkImage Image, VkBuffer Buffer, uint Width, uint Height);

    void GenerateMips(VkImage Image, uint Width, uint Height, uint NumMips);

    void GenerateMips(VkImage Image, uint Width, uint Height, uint NumMips, VkFormat Format);

    auto GetBufferDeviceAddress(VkBuffer Buffer) -> VkDeviceSize;
}  // namespace Shadowy::RHI

#endif //HARDWAREPATHTRACER_RHI_H
