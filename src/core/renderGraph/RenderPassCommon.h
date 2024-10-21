//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RENDERPASSCOMMON_H
#define HARDWAREPATHTRACER_RENDERPASSCOMMON_H

#include "core/Core.h"
#include <vector>


namespace HWPT {
    enum class PassFlag : uint8_t {
        None = 0x0,
        Raster,
        Compute,
        AsyncCompute,  // TODO
    };

    enum class PrimitiveType : uint8_t {
        None = 0x0,
        Point,
        Line,
        LineStrip,
        Triangle,
        TriangleStrip,
        TriangleFan,
        LineWithAdjacency,
        LineStripWithAdjacency,
        TriangleWithAdjacency,
        TriangleStripWithAdjacency,
        Patch
    };

    auto GetVKPrimitiveType(PrimitiveType Type) -> VkPrimitiveTopology;

    class ShaderParameters;

    class RenderPassBase {
    public:
        friend class ShaderParameters;

        explicit RenderPassBase(std::string PassName, PassFlag Flag)
                : m_passName(std::move(PassName)), m_passFlag(Flag) {}

        virtual ~RenderPassBase();

        void CreatePipelineLayout();

        virtual void BindRenderPass(VkCommandBuffer CommandBuffer) = 0;

        virtual void OnRenderPassSetupFinish() = 0;

    public:
        void SetShaderParameters(ShaderParameters* PassParameters);

        void SetCurrentCommandBuffer(VkCommandBuffer CommandBuffer) {
            m_commandBuffer = CommandBuffer;
        }

        [[nodiscard]] auto GetPassName() const -> const std::string & {
            return m_passName;
        }

        auto GetPassFlag() -> PassFlag {
            return m_passFlag;
        }

        auto GetPipelineLayout() -> VkPipelineLayout {
            return m_pipelineLayout;
        }

        auto GetPipeline() -> VkPipeline {
            return m_pipeline;
        }

        auto GetShaderParameters() -> ShaderParameters* {
            return m_parameters;
        }

        auto GetCurrentCommandBuffer() -> VkCommandBuffer& {
            return m_commandBuffer;
        }

    protected:
        std::string m_passName = "Uninitialized";
        PassFlag m_passFlag = PassFlag::None;

        ShaderParameters* m_parameters = nullptr;  // TODO

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        VkPipeline m_pipeline = VK_NULL_HANDLE;

        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RENDERPASSCOMMON_H
