//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RENDERPASSCOMMON_H
#define HARDWAREPATHTRACER_RENDERPASSCOMMON_H

#include "core/Core.h"


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

    class RenderPassBase {
    public:
        explicit RenderPassBase(std::string PassName, PassFlag Flag)
                : m_passName(std::move(PassName)), m_passFlag(Flag) {}

        virtual ~RenderPassBase() = default;

        auto GetPassFlag() -> PassFlag {
            return m_passFlag;
        }

        [[nodiscard]] auto GetPassName() const -> const std::string & {
            return m_passName;
        }

    protected:
        std::string m_passName = "Uninitialized";
        PassFlag m_passFlag = PassFlag::None;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RENDERPASSCOMMON_H
