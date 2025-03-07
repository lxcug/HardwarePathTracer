//
// Created by HUSTLX on 2025/3/8.
//

#ifndef SHADOWY_PICKINGOBJECTHIGHLIGHTPASS_H
#define SHADOWY_PICKINGOBJECTHIGHLIGHTPASS_H

#include "core/Core.h"
#include "core/PassBase.h"


namespace Shadowy {
    class PickingObjectHighlightPass : public PassBase {
    public:
        void Init(uint Width, uint Height) override {
            PassBase::Init(Width, Height);
        }

        void CreateSets() override;

        void UpdateSets() override;

        void CreatePipeline() override;

        void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) override;

        void OnResize(uint Width, uint Height) override {
            PassBase::OnResize(Width, Height);
            UpdateSets();
        }
    };
}


#endif //SHADOWY_PICKINGOBJECTHIGHLIGHTPASS_H
