//
// Created by HUSTLX on 2025/2/26.
//

#ifndef TEXTUREMANAGER_H_H
#define TEXTUREMANAGER_H_H

#include "core/Core.h"
#include "filesystem"
#include "assimp/texture.h"


namespace Shadowy {

    class Texture2D;
    /**
     * Texture Reuse Manager
     * Provide Interface for Create Texture from Path and Get Texture using Path
     */
    class TextureManager {
    public:
        ~TextureManager() = default;

        auto CreateOrRetrieveTexture(const std::filesystem::path& Path) -> const std::tuple<Texture2D*, uint>&;

        auto CreateOrRetrieveTexture(const aiTexture* AITexture) -> const std::tuple<Texture2D*, uint>&;

        [[nodiscard]] auto IsTextureExist(const std::filesystem::path& Path) const -> bool
        {
            return m_storedTextures.find(Path) != m_storedTextures.end();  // NOLINT
        }

        std::unordered_map<std::filesystem::path, std::tuple<Texture2D*, uint>> m_storedTextures;
        std::vector<std::shared_ptr<Texture2D>> m_uniqueTextures;
    };

}  // namespace Shadowy

#endif //TEXTUREMANAGER_H_H
