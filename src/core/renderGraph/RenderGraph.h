//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RENDERGRAPH_H
#define HARDWAREPATHTRACER_RENDERGRAPH_H

#include "RenderPassCommon.h"


namespace HWPT {
    struct RenderPassMetaData {
        std::string PassName;
        RenderPassBase* Pass = nullptr;

        ~RenderPassMetaData() {
            delete Pass;
        }
    };

    class RenderGraph {
    public:

//        void AddPass();

    private:
        std::vector<RenderPassMetaData> m_passMetaDatas;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RENDERGRAPH_H
