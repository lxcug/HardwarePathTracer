//
// Created by HUSTLX on 2025/2/12.
//

#ifndef SHADOWY_GLOBALTEXTURE_H
#define SHADOWY_GLOBALTEXTURE_H

#include "core/Core.h"
#include "core/texture/Texture2D.h"


namespace Shadowy {

    enum class TextureType {
        // GBuffer
        MotionVector = 0x0,
        SceneDepth,
        Albedo_Metallic,
        Normal_Roughness,
        WorldPosition,
        Penumbra,
        Translucency,

        // Lighting
        Diffuse_Radiance_HitDis,
        Specular_Radiance_HitDis,

        Denoised_Diffuse_Radiance_HitDis,
        Denoised_Specular_Radiance_HitDis,
        SceneColor,

        TextureTypeMax
    };

    class GlobalTexture {
    public:
        GlobalTexture() {
            m_textures.resize(static_cast<uint>(TextureType::TextureTypeMax),
                              std::vector<Texture2D*>(MAX_FRAMES_IN_FLIGHT, nullptr));
        }

        ~GlobalTexture() {
            for (auto textures : m_textures) {
                for (auto texture : textures) {
                    delete texture;
                }
            }
        }

        void CreateGlobalTexture(TextureType Type, uint Width, uint Height, VkFormat Format,
                                 VkImageUsageFlags Usage, VkImageLayout InitialLayout);

        void RecreateGlobalTexture(TextureType Type, uint Width, uint Height, VkFormat Format,
                                   VkImageUsageFlags Usage, VkImageLayout InitialLayout) {
            ReleaseGlobalTexture(Type);
            CreateGlobalTexture(Type, Width, Height, Format, Usage, InitialLayout);
        }

        auto GetGlobalTexture(TextureType Type, uint ImageIndex) -> Texture2D* {
            auto Ret = m_textures[static_cast<uint>(Type)][ImageIndex];
            Check(Ret);
            return Ret;
        }

        auto GetGlobalTexture(TextureType Type) -> std::vector<Texture2D*>& {
            return m_textures[static_cast<uint>(Type)];
        }

        void ReleaseGlobalTexture(TextureType Type) {
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                delete m_textures[static_cast<uint>(Type)][i];
                m_textures[static_cast<uint>(Type)][i] = nullptr;
            }
        }

        void ReleaseGlobalTexture(const std::initializer_list<TextureType> &Types) {
            for (auto Type : Types) {
                ReleaseGlobalTexture(Type);
            }
        }

    protected:
        std::vector<std::vector<Texture2D*>> m_textures;
    };

}  // namespace Shadowy

#endif //SHADOWY_GLOBALTEXTURE_H
