//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RASTERPASS_H
#define HARDWAREPATHTRACER_RASTERPASS_H

#include "RenderPassCommon.h"
#include <optional>
#include "core/shader/ShaderBase.h"
#include "core/buffer/VertexBufferLayout.h"
#include "ShaderParameters.h"
#include "core/buffer/VertexBuffer.h"
#include "core/buffer/IndexBuffer.h"
#include "core/Model.h"
#include "core/buffer/StorageBuffer.h"


namespace HWPT {
    struct RasterPassShaders {
        std::optional<ShaderBase *> VertexShader = std::nullopt;
        std::optional<ShaderBase *> GeometryShader = std::nullopt;
        std::optional<ShaderBase *> FragmentShader = std::nullopt;

        ~RasterPassShaders() {
            if (VertexShader.has_value()) {
                delete VertexShader.value();
            }
            if (GeometryShader.has_value()) {
                delete GeometryShader.value();
            }
            if (FragmentShader.has_value()) {
                delete FragmentShader.value();
            }
        }
    };

    class RasterPass : public RenderPassBase {
    public:
        RasterPass(std::string PassName, PassFlag Flag,
                   const std::string &VertexSPVPath,
                   const char *VertexEntry,
                   const std::string &FragSPVPath,
                   const char *FragEntry,
                   bool IsBasePass = false,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        RasterPass(std::string PassName, PassFlag Flag,
                   const std::string &VertexSPVPath,
                   const char *VertexEntry,
                   const std::string &GeometrySPVPath,
                   const char *GeometryEntry,
                   const std::string &FragSPVPath,
                   const char *FragEntry,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        ~RasterPass() override;

        void SetVertexBufferLayout(const std::initializer_list<VertexAttribute> &VertexAttributes);

        void OnRenderPassSetupFinish() override;

        void BindRenderPass(VkCommandBuffer CommandBuffer) override;

        [[nodiscard]] auto HasGeometryShader() const -> bool {
            return m_shaders.GeometryShader.has_value();
        }

        auto GetRenderPass() -> VkRenderPass & {
            return m_renderPass;
        }

        [[nodiscard]] auto IsBasePass() const -> bool {
            return m_isBasePass;
        }

        void Execute(const VertexBuffer &InVertexBuffer);

        void Execute(const StorageBuffer &InStorageBuffer, uint VertexCount);

        void Execute(const VertexBuffer &InVertexBuffer, const IndexBuffer &InIndexBuffer);

        void Execute(const Model &InModel);

    private:
        void CreateDefaultVertexBufferLayout();

        void CreateRenderPass();

        void CreateRenderPipeline();

        void BeginRenderPass(VkCommandBuffer CommandBuffer);

        void EndRenderPass(VkCommandBuffer CommandBuffer);

    private:
        RasterPassShaders m_shaders;
        VertexBufferLayout *m_vertexBufferLayout = nullptr;
        PrimitiveType m_primitiveType = PrimitiveType::None;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;  // TODO: sub-pass support
        bool m_isBasePass = false;
        class SwapChain;
        SwapChain *m_swapChain;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RASTERPASS_H
