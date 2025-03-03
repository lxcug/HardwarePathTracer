//
// Created by HUSTLX on 2025/3/2.
//

#ifndef BLOOMPASS_H
#define BLOOMPASS_H


#include "core/Core.h"
#include "core/PassBase.h"


namespace Shadowy {
    class BloomPass : public PassBase {
    public:
        void Init(uint Width, uint Height) override
        {
            PassBase::Init(Width, Height);

            m_tempBrightness.resize(MAX_FRAMES_IN_FLIGHT);
            for (auto& Tex : m_tempBrightness)
            {
                Tex = new Texture2D(Width, Height,
                    VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_IMAGE_LAYOUT_GENERAL);
            }
        }

        ~BloomPass()
        {
            for (auto Tex : m_tempBrightness)
            {
                delete Tex;
            }
        }

        void DestroyPipeline() override
        {
            PassBase::DestroyPipeline();
            vkDestroyPipeline(GetVKDevice(), m_brightnessExtractPipeline, nullptr);
            vkDestroyPipeline(GetVKDevice(), m_horizontalBlurPipeline, nullptr);
            vkDestroyPipeline(GetVKDevice(), m_verticalBlurPipeline, nullptr);
        }

        void CreateSets() override;

        void UpdateSets() override;

        void CreatePipeline() override;

        void OnResize(uint Width, uint Height) override
        {
            PassBase::OnResize(Width, Height);
            for (auto& Tex : m_tempBrightness)
            {
                delete Tex;
                Tex = new Texture2D(m_width, m_height,
                    VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_IMAGE_LAYOUT_GENERAL);
            }
            UpdateSets();
        }

        void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) override;

    private:
        std::vector<Texture2D*> m_tempBrightness;
        VkPipeline m_brightnessExtractPipeline = VK_NULL_HANDLE;
        VkPipeline m_horizontalBlurPipeline = VK_NULL_HANDLE;
        VkPipeline m_verticalBlurPipeline = VK_NULL_HANDLE;
    };

};  // namespace Shadowy

#endif //BLOOMPASS_H
