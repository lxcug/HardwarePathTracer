//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RENDERGRAPH_H
#define HARDWAREPATHTRACER_RENDERGRAPH_H

#include "RenderPassCommon.h"
#include <functional>
#include "core/renderGraph/ShaderParameters.h"
#include "core/renderGraph/RasterPass.h"
#include "core/renderGraph/ComputePass.h"


namespace HWPT {
    struct RenderPassMetaData {
        std::string PassName;
        RenderPassBase *Pass = nullptr;
        std::function<void()> ExecLambda;
    };

    class RenderGraph {
    public:
        explicit RenderGraph(uint NumConcurrentFrames = 2);

        ~RenderGraph();

        void Create();

        void Destroy();

        void AddPass(const std::string &PassName, RenderPassBase *Pass,
                     const std::function<void()> &ExecLambda);

        template<typename ...Args>
        auto AllocateRasterPass(const std::string &PassName, Args &&... args) {
            auto Pass = new RasterPass(PassName, PassFlag::Raster, std::forward<Args>(args)...);
            m_passNameMap[PassName] = Pass;
            return Pass;
        }

        template<typename ...Args>
        auto AllocateComputePass(const std::string &PassName, Args &&... args) {
            auto Pass = new ComputePass(PassName, PassFlag::Compute, std::forward<Args>(args)...);
            m_passNameMap[PassName] = Pass;
            return Pass;
        }

        auto AllocateParameters(RenderPassBase *Pass,
                                const std::initializer_list<std::pair<std::string, ShaderParameterType>> &InitList) -> ShaderParameters *;

        auto Validate() -> bool;

        void OnNewFrame(uint FrameIndex);

        void Execute();

        template<class PassType>
        auto GetPassByName(const std::string &PassName) -> PassType * {
            if (m_passNameMap.find(PassName) == m_passNameMap.end()) {
                throw std::runtime_error("Nonexistent Pass");
            }
            auto Pass = dynamic_cast<PassType *>(m_passNameMap[PassName]);
            if (!Pass) {
                throw std::runtime_error(
                        "The Pass that is retrieving is not equal to template type");
            }
            return Pass;
        }

        auto GetShaderParameterByName(const std::string &PassName) -> ShaderParameters * {
            if (m_passNameMap.find(PassName) == m_passNameMap.end()) {
                throw std::runtime_error("Nonexistent Pass");
            }
            return m_passNameMap[PassName]->GetShaderParameters();
        }

        auto GetImageAvailableSemaphore(uint FrameIndex) -> VkSemaphore & {
            return m_frameImageAvailable[FrameIndex];
        }

        auto GetImageRenderFinishSemaphore(uint FrameIndex) -> VkSemaphore & {
            return m_frameImageRenderFinish[FrameIndex];
        }

    private:
        uint m_concurrentFrames;
        std::vector<RenderPassMetaData> m_passMetaData;
        std::vector<VkSemaphore> m_passSyncSemaphores;
        std::vector<VkSemaphore> m_frameImageAvailable;
        std::vector<VkSemaphore> m_frameImageRenderFinish;
        uint m_frameIndex = 0;
        std::unordered_map<std::string, RenderPassBase *> m_passNameMap;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RENDERGRAPH_H
