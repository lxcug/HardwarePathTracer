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


namespace HWPT {
    struct RasterPassShaders {
        std::optional<ShaderBase*> VertexShader = std::nullopt;
        std::optional<ShaderBase*> GeometryShader = std::nullopt;
        std::optional<ShaderBase*> FragmentShader = std::nullopt;

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
                   const std::string& VertexSPVPath,
                   const char* VertexEntry,
                   const std::string& FragSPVPath,
                   const char* FragEntry,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        RasterPass(std::string PassName, PassFlag Flag,
                   const std::string& VertexSPVPath,
                   const char* VertexEntry,
                   const std::string& GeometrySPVPath,
                   const char* GeometryEntry,
                   const std::string& FragSPVPath,
                   const char* FragEntry,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        ~RasterPass() override;

        void SetVertexBufferLayout(const std::initializer_list<VertexAttribute>& VertexAttributes);

        void OnRenderPassSetupFinish() override;

        void BindRenderPipeline(VkCommandBuffer CommandBuffer) const override;

        [[nodiscard]] auto HasGeometryShader() const -> bool {
            return m_shaders.GeometryShader.has_value();
        }

    private:
        void Init();

        void CreateDefaultVertexBufferLayout();

        void CreateRenderPass();

    private:
        RasterPassShaders m_shaders;
        VertexBufferLayout* m_vertexBufferLayout = nullptr;
        PrimitiveType m_primitiveType = PrimitiveType::None;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RASTERPASS_H
