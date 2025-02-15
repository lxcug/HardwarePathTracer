//
// Created by HUSTLX on 2025/2/12.
//

#ifndef SHADOWY_ACCUMULATIONPASS_H
#define SHADOWY_ACCUMULATIONPASS_H

#include "core/Core.h"
#include "core/PassBase.h"


namespace Shadowy {

    class AccumulationPass : public PassBase {
    public:
        void CreateSets() override;

        void UpdateSets() override;

        void CreatePipeline() override;

        void OnResize(uint Width, uint Height) override {
            PassBase::OnResize(Width, Height);
            UpdateSets();
        }

        void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) override;

        auto GetRenderOptions() -> AccumulationOptions& {
            return m_options;
        }

    private:
        AccumulationOptions m_options;
    };

}  // namespace Shadowy

#endif //SHADOWY_ACCUMULATIONPASS_H
