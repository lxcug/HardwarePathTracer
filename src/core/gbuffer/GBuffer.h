//
// Created by HUSTLX on 2025/1/14.
//

#ifndef GBUFFER_H
#define GBUFFER_H

#include "core/Core.h"
#include "core/texture/Texture2D.h"
#include "core/RHI.h"


namespace HWPT
{
    class GBuffer
    {
    public:
        GBuffer(uint FrameCount, uint Width, uint Height) : m_frameCount(FrameCount),
                                                            m_size(Width, Height)
        {
            Init();
        }

        GBuffer(uint FrameCount, const glm::vec2& Size) : m_frameCount(FrameCount),
                                                          m_size(Size)
        {
            Init();
        }

        void Init();

        ~GBuffer()
        {
            Release();
        }

        void Release();

        void OnResize(uint Width, uint Height)
        {
            m_size = {Width, Height};
            Release();
            Init();
        }

        void OnResize(const glm::vec2& Size)
        {
            m_size = Size;
            Release();
            Init();
        }

        [[nodiscard]] auto GetGBufferAlbedo(uint ImageIndex) const -> Texture2D*
        {
            Check(ImageIndex < m_frameCount);
            return m_albedo[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferNormal(uint ImageIndex) const -> Texture2D*
        {
            Check(ImageIndex < m_frameCount);
            return m_normal[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferPos(uint ImageIndex) const -> Texture2D*
        {
            Check(ImageIndex < m_frameCount);
            return m_pos[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferDepth(uint ImageIndex) const -> Texture2D*
        {
            Check(ImageIndex < m_frameCount);
            return m_depth[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferAlbedoDescriptorSet(uint ImageIndex) const -> VkDescriptorSet
        {
            return m_albedoDescriptorSets[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferNormalDescriptorSet(uint ImageIndex) const -> VkDescriptorSet
        {
            return m_normalDescriptorSets[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferPosDescriptorSet(uint ImageIndex) const -> VkDescriptorSet
        {
            return m_posDescriptorSets[ImageIndex];
        }

        [[nodiscard]] auto GetGBufferDepthDescriptorSet(uint ImageIndex) const -> VkDescriptorSet
        {
            return m_depthDescriptorSets[ImageIndex];
        }

    protected:
        uint m_frameCount = 2;
        glm::vec2 m_size = glm::vec2(1.f, 1.f);
        std::vector<Texture2D*> m_albedo;
        std::vector<Texture2D*> m_normal;
        std::vector<Texture2D*> m_pos;
        std::vector<Texture2D*> m_depth;
        std::vector<VkDescriptorSet> m_albedoDescriptorSets;
        std::vector<VkDescriptorSet> m_normalDescriptorSets;
        std::vector<VkDescriptorSet> m_posDescriptorSets;
        std::vector<VkDescriptorSet> m_depthDescriptorSets;
    };
} // namespace HWPT

#endif //GBUFFER_H
