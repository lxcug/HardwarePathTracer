//
// Created by HUSTLX on 2025/2/26.
//

#include "TextureManager.h"
#include "core/texture/Texture2D.h"


namespace Shadowy {

    auto TextureManager::CreateOrRetrieveTexture(
        const std::filesystem::path& Path) -> const std::tuple<Texture2D*, uint>&
    {
        if (IsTextureExist(Path))
        {
            return m_storedTextures[Path];
        }

        const auto Tex = std::make_shared<Texture2D>(Path);
        m_storedTextures[Path] = {Tex.get(), m_uniqueTextures.size()};
        m_uniqueTextures.push_back(Tex);

        return m_storedTextures[Path];
    }

    auto TextureManager::CreateOrRetrieveTexture(
        const aiTexture* AITexture) -> const std::tuple<Texture2D*, uint>&
    {
        const std::filesystem::path Path = AITexture->mFilename.C_Str();
        if (IsTextureExist(Path))
        {
            return m_storedTextures[Path];
        }

        const auto Tex = std::make_shared<Texture2D>(AITexture);
        m_storedTextures[Path] = {Tex.get(), m_uniqueTextures.size()};
        m_uniqueTextures.push_back(Tex);

        return m_storedTextures[Path];
    }
}  // namespace Shadowy
