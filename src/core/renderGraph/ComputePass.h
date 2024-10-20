//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_COMPUTEPASS_H
#define HARDWAREPATHTRACER_COMPUTEPASS_H

#include "core/renderGraph/RenderPassCommon.h"
#include "core/shader/ShaderBase.h"


namespace HWPT {
    class ComputePass : public RenderPassBase {
    public:
        ComputePass(std::string PassName, PassFlag Flag,
                    const std::string &ComputeSPVPath,
                    const char *ComputeEntry);

        ~ComputePass() override;

        void OnRenderPassSetupFinish() override;

        void BindRenderPass(VkCommandBuffer CommandBuffer) const override;

        // TODO: Get ThreadGroupSize From Shader
        void Execute(VkCommandBuffer CommandBuffer, uint NumThreadGroupX, uint NumThreadGroupY,
                     uint NumThreadGroupZ);

    private:
        void CreateRenderPipeline();

    private:
        ShaderBase *m_computeShader = nullptr;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_COMPUTEPASS_H
